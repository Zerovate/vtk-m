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

#include <vtkm/cont/CubicHermiteSpline.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace
{

class SplineEvalWorklet : public vtkm::worklet::WorkletMapField
{
public:
  SplineEvalWorklet() {}

  using ControlSignature = void(FieldIn param, ExecObject cubicSpline, FieldOut value);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename ParamType, typename CubicSplineType, typename ResultType>
  VTKM_EXEC void operator()(const ParamType& param,
                            const CubicSplineType& spline,
                            ResultType& value) const
  {
    auto res = spline.Evaluate(param, value);
    if (res != vtkm::ErrorCode::Success)
      throw vtkm::cont::ErrorBadValue("Spline evaluation failed.");
  }
};

template <typename SplineType, typename T>
void CheckEvaluation(const SplineType& spline,
                     const std::vector<vtkm::FloatDefault>& params,
                     const std::vector<T>& answer)
{
  auto paramsAH = vtkm::cont::make_ArrayHandle(params, vtkm::CopyFlag::Off);

  vtkm::cont::Invoker invoke;
  vtkm::cont::ArrayHandle<T> result;

  invoke(SplineEvalWorklet{}, paramsAH, spline, result);

  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result, vtkm::cont::make_ArrayHandle(answer, vtkm::CopyFlag::Off)));
}

void CubicHermiteSplineTest()
{
  std::vector<vtkm::Vec3f> pts = { { 0, 0, 0 },  { 1, 1, 0 },  { 2, 1, 0 }, { 3, -.5, 0 },
                                   { 4, -1, 0 }, { 5, -1, 0 }, { 6, 0, 0 } };

  std::vector<vtkm::FloatDefault> knots;
  pts.clear();

  vtkm::Id n = 100;
  vtkm::FloatDefault t = 0.0, dt = vtkm::TwoPi() / static_cast<vtkm::FloatDefault>(n);

  while (t <= vtkm::TwoPi())
  {
    vtkm::FloatDefault x = vtkm::Cos(t);
    vtkm::FloatDefault y = vtkm::Sin(t);
    vtkm::FloatDefault z = vtkm::Cos(t) * vtkm::Sin(t);
    pts.push_back({ x, y, z });
    knots.push_back(t);
    t += dt;
  }
  vtkm::cont::CubicHermiteSpline spline0(pts, knots);
  auto parametricRange = spline0.GetParametricRange();

  t = parametricRange.Min;
  dt = parametricRange.Length() / 1500;
  std::vector<vtkm::FloatDefault> vals;
  while (t <= parametricRange.Max)
  {
    vals.push_back(t);
    t += dt;
  }
  std::cout << "KNOTS= ";
  vtkm::cont::printSummary_ArrayHandle(spline0.GetKnots(), std::cout);

  vtkm::cont::Invoker invoke;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> result;
  invoke(
    SplineEvalWorklet{}, vtkm::cont::make_ArrayHandle(vals, vtkm::CopyFlag::Off), spline0, result);

  std::cout << "SaveSamples" << std::endl;
  std::ofstream fout("/Users/dpn/output.txt"), ptsOut("/Users/dpn/pts.txt");
  fout << "T, X, Y, Z" << std::endl;
  ptsOut << "T, X, Y, Z" << std::endl;
  auto knots_ = spline0.GetKnots().ReadPortal();
  for (vtkm::Id i = 0; i < pts.size(); i++)
    ptsOut << knots_.Get(i) << ", " << pts[i][0] << ", " << pts[i][1] << ", " << pts[i][2]
           << std::endl;

  auto res = result.ReadPortal();
  for (vtkm::Id i = 0; i < res.GetNumberOfValues(); i++)
    fout << vals[i] << ", " << res.Get(i)[0] << ", " << res.Get(i)[1] << ", " << res.Get(i)[2]
         << std::endl;
}

} // anonymous namespace

int UnitTestCubicHermiteSpline(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(CubicHermiteSplineTest, argc, argv);
}
