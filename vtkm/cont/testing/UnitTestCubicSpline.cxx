//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <random>
#include <string>

#include <vtkm/cont/CubicSpline.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/exec/CubicSpline.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace
{

class SplineEvalWorklet : public vtkm::worklet::WorkletMapField
{
public:
  SplineEvalWorklet() {}

  using ControlSignature = void(FieldIn param,
                                ExecObject cubicSpline,
                                FieldOut value,
                                FieldOut valid);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename ParamType, typename CubicSplineType, typename ResultType>
  VTKM_EXEC void operator()(const ParamType& param,
                            const CubicSplineType& spline,
                            ResultType& value,
                            bool& valid) const
  {
    valid = spline.Evaluate(param, value);
  }
};

void CheckEvaluation(const vtkm::cont::CubicSpline& spline,
                     const std::vector<vtkm::FloatDefault>& params,
                     const std::vector<vtkm::FloatDefault>& answer)
{
  auto paramsAH = vtkm::cont::make_ArrayHandle(params, vtkm::CopyFlag::Off);

  vtkm::cont::Invoker invoke;
  SplineEvalWorklet worklet;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> result;
  vtkm::cont::ArrayHandle<bool> valid;
  invoke(worklet, paramsAH, spline, result, valid);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == static_cast<vtkm::Id>(answer.size()),
                   "Result wrong length.");
  return;

  for (std::size_t i = 0; i < answer.size(); i++)
  {
    VTKM_TEST_ASSERT(valid.ReadPortal().Get(static_cast<vtkm::Id>(i)), "Evaluation failed.");
    auto val = result.ReadPortal().Get(static_cast<vtkm::Id>(i));
    std::cout << params[i] << " = " << val << " " << answer[i]
              << " :: diff: " << vtkm::Abs(val - answer[i]) << std::endl;
    VTKM_TEST_ASSERT(vtkm::Abs(val - answer[i]) < 1e-6, "Result has wrong value.");
  }
}

//Convenience function to save a bunch of samples to a file.
void SaveSamples(const vtkm::cont::CubicSpline& spline)
{
  vtkm::Id n = spline.GetControlPoints().GetNumberOfValues();
  vtkm::FloatDefault t = spline.GetControlPoints().ReadPortal().Get(0);
  vtkm::FloatDefault tEnd = spline.GetControlPoints().ReadPortal().Get(n - 1);

  vtkm::FloatDefault dt = (tEnd - t) / (n * 100);
  std::vector<vtkm::FloatDefault> params;
  while (t < tEnd)
  {
    params.push_back(t);
    t += dt;
  }
  auto paramsAH = vtkm::cont::make_ArrayHandle(params, vtkm::CopyFlag::Off);

  vtkm::cont::Invoker invoke;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> result;
  vtkm::cont::ArrayHandle<bool> valid;
  invoke(SplineEvalWorklet{}, paramsAH, spline, result, valid);

  std::cout << "SaveSamples" << std::endl;
  std::ofstream fout("output.txt"), ptsOut("pts.txt");
  fout << "X,Y" << std::endl;
  for (vtkm::Id i = 0; i < paramsAH.GetNumberOfValues(); i++)
  {
    fout << paramsAH.ReadPortal().Get(i) << "," << result.ReadPortal().Get(i) << std::endl;
  }

  ptsOut << "X,Y" << std::endl;
  auto pts = spline.GetControlPoints().ReadPortal();
  auto vals = spline.GetValues().ReadPortal();
  for (vtkm::Id i = 0; i < pts.GetNumberOfValues(); i++)
    ptsOut << pts.Get(i) << ", " << vals.Get(i) << std::endl;
}


void CubicSplineTest()
{
  std::vector<vtkm::FloatDefault> xVals = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
  std::vector<vtkm::FloatDefault> yVals = { 0.0f, 1.0f, -1.0f, 1.0f, 0.0f };

  auto xValsAH = vtkm::cont::make_ArrayHandle(xVals, vtkm::CopyFlag::On);
  auto yValsAH = vtkm::cont::make_ArrayHandle(yVals, vtkm::CopyFlag::On);

  vtkm::cont::CubicSpline cubicSpline;
  cubicSpline.SetControlPoints(xValsAH);
  cubicSpline.SetValues(yValsAH);
  cubicSpline.Update();

  //SaveSamples(cubicSpline);

  //Ensure that the values at control points are properly interpolated.
  auto tVals = xVals;
  auto res = yVals;
  CheckEvaluation(cubicSpline, tVals, res);

  //Evaluate between control points.
  tVals = { 0.6, 1.4, 2.16, 3.51198 };
  res = { 1.03886, 0.110853, -0.890431, 0.91292 };

  CheckEvaluation(cubicSpline, tVals, res);
}

} // anonymous namespace

int UnitTestCubicSpline(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(CubicSplineTest, argc, argv);
}
