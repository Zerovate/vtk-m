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

class Spline
{
public:
  // Constructor: expects x and y to have the same size and at least 2 points.
  Spline(const std::vector<vtkm::FloatDefault>& x, const std::vector<vtkm::FloatDefault>& y)
    : x_(x)
    , y_(y)
    , n_(x.size())
  {
    if (n_ < 2)
    {
      throw std::invalid_argument("At least two points are required for spline interpolation.");
    }
    if (x_.size() != y_.size())
    {
      throw std::invalid_argument("x and y must have the same length.");
    }
    computeCoefficients();
  }

  // Evaluate the spline at a given x value.
  vtkm::FloatDefault evaluate(vtkm::FloatDefault x_val) const
  {
    // Ensure x_val is within the interpolation range.
    if (x_val < x_.front() || x_val > x_.back())
    {
      throw std::out_of_range("x_val is outside the interpolation range.");
    }
    // Find the interval index i such that x_[i] <= x_val <= x_[i+1].
    size_t i = std::upper_bound(x_.begin(), x_.end(), x_val) - x_.begin() - 1;
    vtkm::FloatDefault dx = x_val - x_[i];
    return a_[i] + b_[i] * dx + c_[i] * dx * dx + d_[i] * dx * dx * dx;
  }

private:
  std::vector<vtkm::FloatDefault> x_; // x coordinates
  std::vector<vtkm::FloatDefault> y_; // y coordinates
  size_t n_;                          // number of points

  // Spline coefficients for each interval i = 0, ..., n-2.
  // For each segment, the cubic polynomial is:
  // S_i(x) = a_[i] + b_[i]*(x-x_[i]) + c_[i]*(x-x_[i])^2 + d_[i]*(x-x_[i])^3
  std::vector<vtkm::FloatDefault> a_, b_, c_, d_;

  // Compute the spline coefficients using the standard algorithm.
  void computeCoefficients()
  {
    // Natural cubic spline: second derivatives at endpoints are zero.
    // Step 1: Compute h[i] = x[i+1] - x[i] for i = 0 ... n-2.
    std::vector<vtkm::FloatDefault> h(n_ - 1);
    for (size_t i = 0; i < n_ - 1; ++i)
    {
      h[i] = x_[i + 1] - x_[i];
    }

    // Step 2: Compute the array 'alpha' for i = 1 ... n-2:
    // alpha[i] = 3/h[i]*(y[i+1]-y[i]) - 3/h[i-1]*(y[i]-y[i-1])
    std::vector<vtkm::FloatDefault> alpha(n_, 0.0);
    for (size_t i = 1; i < n_ - 1; ++i)
    {
      alpha[i] = (3.0 / h[i]) * (y_[i + 1] - y_[i]) - (3.0 / h[i - 1]) * (y_[i] - y_[i - 1]);
    }

    // Step 3: Set up the tridiagonal system and solve it.
    std::vector<vtkm::FloatDefault> l(n_), mu(n_), z(n_);
    l[0] = 1.0;
    mu[0] = z[0] = 0.0;
    for (size_t i = 1; i < n_ - 1; ++i)
    {
      l[i] = 2.0 * (x_[i + 1] - x_[i - 1]) - h[i - 1] * mu[i - 1];
      mu[i] = h[i] / l[i];
      z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
    }
    l[n_ - 1] = 1.0;
    z[n_ - 1] = 0.0;
    c_.resize(n_);
    c_[n_ - 1] = 0.0;

    // Step 4: Back substitution to solve for c[i].
    for (int j = n_ - 2; j >= 0; --j)
    {
      c_[j] = z[j] - mu[j] * c_[j + 1];
    }

    // Step 5: Compute the coefficients b and d for each interval.
    a_.resize(n_ - 1);
    b_.resize(n_ - 1);
    d_.resize(n_ - 1);
    for (size_t i = 0; i < n_ - 1; ++i)
    {
      a_[i] = y_[i]; // a[i] is simply the y value at x[i]
      b_[i] = (y_[i + 1] - y_[i]) / h[i] - h[i] * (c_[i + 1] + 2.0 * c_[i]) / 3.0;
      d_[i] = (c_[i + 1] - c_[i]) / (3.0 * h[i]);
    }
  }
};

class Spline2
{
public:
  Spline2(const std::vector<vtkm::FloatDefault>& x, const std::vector<vtkm::FloatDefault>& y)
    : x1D(x)
    , y1D(y)
    , n(x.size())
  {
    if (x1D.size() != y1D.size())
      throw std::invalid_argument("x and y must have the same length.");
    if (n < 2)
      throw std::invalid_argument("At least two points are required for spline interpolation.");
  }

  // Evaluate the spline at a given x value.
  vtkm::FloatDefault evaluate(vtkm::FloatDefault x_val) const
  {
    // Compute the spline coefficients on the fly.
    Coefs coefs = computeCoefficients();

    // Ensure x_val is within the interpolation range.
    if (x_val < x1D.front() || x_val > x1D.back())
      throw std::out_of_range("x_val is outside the interpolation range.");

    // Find the interval index i such that x1D[i] <= x_val <= x1D[i+1]
    size_t i = findInterval(x1D, x_val);

    vtkm::FloatDefault dx = x_val - x1D[i];
    // Evaluate S_i(x) = a[i] + b[i]*(x - x_i) + c[i]*(x - x_i)^2 + d[i]*(x - x_i)^3
    return coefs.a[i] + coefs.b[i] * dx + coefs.c[i] * dx * dx + coefs.d[i] * dx * dx * dx;
  }

private:
  std::vector<vtkm::FloatDefault> x1D, y1D;
  size_t n;

  // Structure to hold the spline coefficients for each interval.
  struct Coefs
  {
    std::vector<vtkm::FloatDefault> a;
    std::vector<vtkm::FloatDefault> b;
    std::vector<vtkm::FloatDefault> c;
    std::vector<vtkm::FloatDefault> d;
  };

  // Compute the cubic spline coefficients using the natural cubic spline algorithm.
  // This function computes the full set of coefficients for all segments.
  Coefs computeCoefficients() const
  {
    Coefs coefs;
    coefs.a = y1D; // a[i] = y1D[i]

    // Allocate coefficient arrays for each segment (there are n-1 segments)
    coefs.b.resize(n - 1);
    coefs.d.resize(n - 1);
    coefs.c.resize(n); // second derivatives for each point

    // Step 1: Compute h[i] = x1D[i+1] - x1D[i]
    std::vector<vtkm::FloatDefault> h(n - 1);
    for (size_t i = 0; i < n - 1; ++i)
    {
      h[i] = x1D[i + 1] - x1D[i];
    }

    // Step 2: Compute alpha[i] for i=1..n-2
    std::vector<vtkm::FloatDefault> alpha(n, 0.0);
    for (size_t i = 1; i < n - 1; ++i)
    {
      alpha[i] = (3.0 / h[i]) * (y1D[i + 1] - y1D[i]) - (3.0 / h[i - 1]) * (y1D[i] - y1D[i - 1]);
    }

    // Step 3: Set up and solve the tridiagonal system
    std::vector<vtkm::FloatDefault> l(n, 0.0), mu(n, 0.0), z(n, 0.0);
    l[0] = 1.0;
    mu[0] = 0.0;
    z[0] = 0.0;
    for (size_t i = 1; i < n - 1; ++i)
    {
      l[i] = 2.0 * (x1D[i + 1] - x1D[i - 1]) - h[i - 1] * mu[i - 1];
      mu[i] = h[i] / l[i];
      z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
    }
    l[n - 1] = 1.0;
    z[n - 1] = 0.0;
    coefs.c[n - 1] = 0.0;

    // Step 4: Back substitution to solve for c[i]
    for (int j = n - 2; j >= 0; --j)
    {
      coefs.c[j] = z[j] - mu[j] * coefs.c[j + 1];
    }

    // Step 5: Compute b[i] and d[i] for i=0..n-2
    for (size_t i = 0; i < n - 1; ++i)
    {
      coefs.b[i] = (y1D[i + 1] - y1D[i]) / h[i] - h[i] * (coefs.c[i + 1] + 2.0 * coefs.c[i]) / 3.0;
      coefs.d[i] = (coefs.c[i + 1] - coefs.c[i]) / (3.0 * h[i]);
    }

    return coefs;
  }

  // Find the interval index for a given value.
  size_t findInterval(const std::vector<vtkm::FloatDefault>& x, vtkm::FloatDefault val) const
  {
    if (val < x.front() || val > x.back())
      throw std::out_of_range("Value is outside the interpolation range.");
    size_t idx = std::upper_bound(x.begin(), x.end(), val) - x.begin() - 1;
    return (idx >= x.size() - 1) ? x.size() - 2 : idx;
  }
};

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

void SaveSamples(const vtkm::cont::CubicSpline& spline_)
{
  vtkm::Id n = spline_.GetControlPoints().GetNumberOfValues();
  vtkm::FloatDefault t = spline_.GetControlPoints().ReadPortal().Get(0);
  vtkm::FloatDefault tEnd = spline_.GetControlPoints().ReadPortal().Get(n - 1);

  std::vector<vtkm::FloatDefault> x1D, y1D;
  for (vtkm::Id i = 0; i < n; i++)
  {
    x1D.push_back(spline_.GetControlPoints().ReadPortal().Get(i));
    y1D.push_back(spline_.GetValues().ReadPortal().Get(i));
  }

  //Spline spline(x1D, y1D);
  Spline2 spline(x1D, y1D);

  vtkm::FloatDefault dt = (tEnd - t) / (n * 100);
  std::vector<vtkm::FloatDefault> params;
  while (t < tEnd)
  {
    params.push_back(t);
    t += dt;
  }

  /*
  auto paramsAH = vtkm::cont::make_ArrayHandle(params, vtkm::CopyFlag::Off);

  vtkm::cont::Invoker invoke;
  SplineEvalWorklet worklet;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> result;
  vtkm::cont::ArrayHandle<bool> valid;
  invoke(worklet, paramsAH, spline, result, valid);
*/

  std::cout << "SaveSamples" << std::endl;
  std::ofstream fout("/Users/dpn/output.txt"), ptsOut("/Users/dpn/pts.txt");
  fout << "X,Y" << std::endl;
  for (std::size_t i = 0; i < params.size(); i++)
  {
    fout << params[i] << "," << spline.evaluate(params[i]) << std::endl;
    //fout << params[i] << "," << result.ReadPortal().Get(i) << std::endl;
  }

  ptsOut << "X,Y" << std::endl;
  for (std::size_t i = 0; i < x1D.size(); i++)
    ptsOut << x1D[i] << ", " << y1D[i] << std::endl;
}


void CubicSplineTest()
{
  //std::vector<vtkm::FloatDefault> xVals = { 0.0f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f };
  //std::vector<vtkm::FloatDefault> yVals = { 0.0f, -1.0f, 2.0f, 1.0f, -1.0f, 0.0f };
  std::vector<vtkm::FloatDefault> xVals = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
  std::vector<vtkm::FloatDefault> yVals = { 0.0f, 1.0f, 4.4f, 1.0f, 0.0f };

  auto xValsAH = vtkm::cont::make_ArrayHandle(xVals, vtkm::CopyFlag::On);
  auto yValsAH = vtkm::cont::make_ArrayHandle(yVals, vtkm::CopyFlag::On);

  vtkm::cont::CubicSpline cubicSpline;
  cubicSpline.SetControlPoints(xValsAH);
  cubicSpline.SetValues(yValsAH);
  cubicSpline.Update();

  SaveSamples(cubicSpline);


  //Ensure that the values at control points are properly interpolated.
  auto tVals = xVals;
  auto res = yVals;
  CheckEvaluation(cubicSpline, tVals, res);

  /*
  //Evaluate between control points.
  tVals = { 0.5, 1.3, 2.1, 3.7 };
  res = { -1.68413, 1.00957, 3.02636, -0.461989 };
  CheckEvaluation(cubicSpline, tVals, res);
  */
}

} // anonymous namespace

int UnitTestCubicSpline(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(CubicSplineTest, argc, argv);
}
