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

void CheckEvaluation(const vtkm::cont::CubicHermiteSpline& spline,
                     const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& params,
                     const std::vector<vtkm::Vec3f>& answer)
{
  vtkm::cont::Invoker invoke;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> result;
  invoke(SplineEvalWorklet{}, params, spline, result);

  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result, vtkm::cont::make_ArrayHandle(answer, vtkm::CopyFlag::Off)));
}

void CheckEvaluation(const vtkm::cont::CubicHermiteSpline& spline,
                     const std::vector<vtkm::FloatDefault>& params,
                     const std::vector<vtkm::Vec3f>& answer)
{
  return CheckEvaluation(spline, vtkm::cont::make_ArrayHandle(params, vtkm::CopyFlag::Off), answer);
}

void CubicHermiteSplineTest()
{
  std::vector<vtkm::Vec3f> pts = { { 0, 0, 0 },  { 1, 1, 0 },  { 2, 1, 0 }, { 3, -.5, 0 },
                                   { 4, -1, 0 }, { 5, -1, 0 }, { 6, 0, 0 } };

  vtkm::cont::CubicHermiteSpline spline(pts);
  //Evaluation at knots gives the sample pts.
  CheckEvaluation(spline, spline.GetKnots(), pts);

  //Evaluation at non-knot values.
  std::vector<vtkm::FloatDefault> params = { 0.21, 0.465, 0.501, 0.99832 };
  std::vector<vtkm::Vec3f> result = { { 1.20072, 1.07379, 0 },
                                      { 2.64681, 0.012108, 0 },
                                      { 2.79356, -0.239254, 0 },
                                      { 5.99081, -0.00924034, 0 } };
  CheckEvaluation(spline, params, result);

  //Explicitly set knots and check.
  std::vector<vtkm::FloatDefault> knots = { 0, 1, 2, 3, 4, 5, 6 };
  spline = vtkm::cont::CubicHermiteSpline(pts, knots);
  CheckEvaluation(spline, knots, pts);

  //Non-uniform knots.
  knots = { 0, 1, 2, 2.1, 2.2, 2.3, 3 };
  spline = vtkm::cont::CubicHermiteSpline(pts, knots);
  CheckEvaluation(spline, knots, pts);

  params = { 1.5, 2.05, 2.11, 2.299, 2.8 };
  result = { { 1.39773, 1.23295, 0 },
             { 2.39773, 0.357954, 0 },
             { 3.1, -0.59275, 0 },
             { 4.99735, -1.00125, 0 },
             { 5.75802, -0.293003, 0 } };
  CheckEvaluation(spline, params, result);

  //Create a more complex spline from analytical functions.
  vtkm::Id n = 500;
  vtkm::FloatDefault t = 0.0, dt = vtkm::TwoPi() / static_cast<vtkm::FloatDefault>(n);

  pts.clear();
  knots.clear();
  while (t <= vtkm::TwoPi())
  {
    vtkm::FloatDefault x = vtkm::Cos(t);
    vtkm::FloatDefault y = vtkm::Sin(t);
    vtkm::FloatDefault z = vtkm::Cos(t) * vtkm::Sin(t);
    pts.push_back({ x, y, z });
    knots.push_back(t);
    t += dt;
  }
  spline = vtkm::cont::CubicHermiteSpline(pts, knots);
  CheckEvaluation(spline, knots, pts);

  params = { 0.15, 2.38, 4.92, 6.2 };
  result.clear();
  for (const auto& p : params)
    result.push_back({ vtkm::Cos(p), vtkm::Sin(p), vtkm::Cos(p) * vtkm::Sin(p) });
  CheckEvaluation(spline, params, result);
}

} // anonymous namespace

int UnitTestCubicHermiteSpline(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(CubicHermiteSplineTest, argc, argv);
}
