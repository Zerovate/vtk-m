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
#include <vtkm/cont/CubicSpline.h>
#include <vtkm/cont/ErrorBadValue.h>
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

//Convenience function to save a bunch of samples to a file.
void SaveSamples(const std::vector<vtkm::FloatDefault>& x, const std::vector<vtkm::FloatDefault>& y)
{
  vtkm::cont::CubicSpline cubicSpline;
  cubicSpline.SetControlPoints(vtkm::cont::make_ArrayHandle(x, vtkm::CopyFlag::Off));
  cubicSpline.SetValues(vtkm::cont::make_ArrayHandle(y, vtkm::CopyFlag::Off));
  cubicSpline.Update();

  vtkm::Id n = cubicSpline.GetControlPoints().GetNumberOfValues();
  vtkm::FloatDefault t = x[0];
  vtkm::FloatDefault tEnd = x[x.size() - 1];

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
  invoke(SplineEvalWorklet{}, paramsAH, cubicSpline, result);

  std::cout << "SaveSamples" << std::endl;
  std::ofstream fout("output.txt"), ptsOut("pts.txt");
  fout << "X,Y" << std::endl;
  for (vtkm::Id i = 0; i < paramsAH.GetNumberOfValues(); i++)
  {
    fout << paramsAH.ReadPortal().Get(i) << "," << result.ReadPortal().Get(i) << std::endl;
  }

  ptsOut << "X,Y" << std::endl;
  auto pts = cubicSpline.GetControlPoints().ReadPortal();
  auto vals = cubicSpline.GetValues().ReadPortal();
  for (vtkm::Id i = 0; i < pts.GetNumberOfValues(); i++)
    ptsOut << pts.Get(i) << ", " << vals.Get(i) << std::endl;
}

void DoTest(const std::vector<vtkm::FloatDefault>& x,
            const std::vector<vtkm::FloatDefault>& y,
            const std::vector<vtkm::FloatDefault>& xSamples,
            const std::vector<vtkm::FloatDefault>& vals)
{
  /*
  vtkm::cont::CubicSpline cubicSpline;
  cubicSpline.SetControlPoints(vtkm::cont::make_ArrayHandle(x, vtkm::CopyFlag::Off));
  cubicSpline.SetValues(vtkm::cont::make_ArrayHandle(y, vtkm::CopyFlag::Off));
  cubicSpline.Update();

  //Make sure we the spline interpolates the control points.
  CheckEvaluation(cubicSpline, x, y);

  //Make sure the spline evaluates to the correct values at the given points.
  CheckEvaluation(cubicSpline, xSamples, vals);
  */
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

  /*
  vtkm::cont::CubicHermiteSpline spline(pts);

  vtkm::Id n = spline.GetData().GetNumberOfValues();
  auto t0 = spline.GetKnots().ReadPortal().Get(0);
  auto t1 = spline.GetKnots().ReadPortal().Get(n-1);

  vtkm::FloatDefault dt = (t1 - t0) / (n * 100);
  std::vector<vtkm::FloatDefault> params;
  vtkm::FloatDefault t = t0;

  while (t < t1)
  {
    params.push_back(t);
    t += dt;
  }

  vtkm::cont::Invoker invoke;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> result;
  invoke(SplineEvalWorklet{}, vtkm::cont::make_ArrayHandle(params, vtkm::CopyFlag::Off), spline, result);
  */

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

void CubicSplineTest()
{
  CubicHermiteSplineTest();

  //uniform spacing wavy function
  std::vector<vtkm::FloatDefault> xVals = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
  std::vector<vtkm::FloatDefault> yVals = { 0.0f, 1.0f, -1.0f, 1.0f, 0.0f };
  std::vector<vtkm::FloatDefault> samples = { 0.6, 1.4, 2.16, 3.51198 };
  std::vector<vtkm::FloatDefault> res = { 1.03886, 0.110853, -0.890431, 0.91292 };

  //DoTest(xVals, yVals, samples, res);

  //non-uniform spacing wavy function
  xVals = { 0.0f, 1.0f, 1.1f, 3.9f, 4.0f, 5.0f };
  yVals = { 0.0f, 1.0f, 1.2f, 1.0f, 0.0f, 0.0f };
  samples = { 0.7f, 1.45f, 3.65f, 4.38f };
  res = { 0.548318, 2.15815f, 3.13104f, -1.77143 };
  //DoTest(xVals, yVals, samples, res);
}

} // anonymous namespace

int UnitTestCubicSpline(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(CubicSplineTest, argc, argv);
}
