//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/CubicSpline.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{

void CubicSpline::Update() const
{
  if (this->Modified)
  {
    // Although the data of the derived class may change, the logical state
    // of the class should not. Thus, we will instruct the compiler to relax
    // the const constraint.
    const_cast<CubicSpline*>(this)->Build();
    this->Modified = false;
  }
}

vtkm::cont::ArrayHandle<vtkm::FloatDefault> CubicSpline::CalcH() const
{
  vtkm::Id n = this->ControlPoints.GetNumberOfValues();
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> h;
  h.Allocate(n - 1);

  auto hPortal = h.WritePortal();
  auto cPortal = this->ControlPoints.ReadPortal();
  for (vtkm::Id i = 0; i < n - 1; i++)
    hPortal.Set(i, cPortal.Get(i + 1) - cPortal.Get(i));

  return h;
}

vtkm::cont::ArrayHandle<vtkm::FloatDefault> CubicSpline::CalcAlpha(
  const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& h) const
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> alpha;
  vtkm::Id n = this->ControlPoints.GetNumberOfValues();
  alpha.Allocate(n - 2);

  auto Alpha = alpha.WritePortal();
  auto H = h.ReadPortal();
  auto Y = this->Values.ReadPortal();

  for (vtkm::Id i = 1; i < n - 1; ++i)
  {
    //alpha[i - 1] = (3.0 * (a1D[i + 1] - a1D[i]) / h[i]) - (3.0 * (a1D[i] - a1D[i - 1]) / h[i - 1]);
    Alpha.Set(i - 1,
              (3.0 * (Y.Get(i + 1) - Y.Get(i)) / H.Get(i)) -
                (3.0 * (Y.Get(i) - Y.Get(i - 1)) / H.Get(i - 1)));
  }

  return alpha;
}

void CubicSpline::CalcCoefficients(const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& H,
                                   const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& Alpha)
{
  vtkm::Id n = this->ControlPoints.GetNumberOfValues();
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> L, Mu, Z;

  L.Allocate(n);
  Mu.Allocate(n);
  Z.Allocate(n);

  auto l = L.WritePortal();
  auto mu = Mu.WritePortal();
  auto z = Z.WritePortal();
  auto h = H.ReadPortal();
  auto alpha = Alpha.ReadPortal();

  l.Set(0, 1.0);
  mu.Set(0, 0.0);
  z.Set(0, 0.0);

  auto xVals = this->ControlPoints.ReadPortal();

  for (vtkm::Id i = 1; i < n - 1; ++i)
  {
    //l[i] = 2.0 * (x1D[i + 1] - x1D[i - 1]) - h[i - 1] * mu[i - 1];
    l.Set(i, 2.0 * (xVals.Get(i + 1) - xVals.Get(i - 1)) - h.Get(i - 1) * mu.Get(i - 1));
    //mu[i] = h[i] / l[i];
    mu.Set(i, h.Get(i) / l.Get(i));
    //z[i] = (alpha[i - 1] - h[i - 1] * z[i - 1]) / l[i];
    z.Set(i, (alpha.Get(i - 1) - h.Get(i - 1) * z.Get(i - 1)) / l.Get(i));
  }

  //l[n - 1] = 1.0;
  //z[n - 1] = c1D[n - 1] = 0.0;
  l.Set(n - 1, 1.0);
  z.Set(n - 1, 0.0);

  this->CoefficientsB.Allocate(n - 1);
  this->CoefficientsC.Allocate(n);
  this->CoefficientsD.Allocate(n - 1);

  auto B = this->CoefficientsB.WritePortal();
  auto C = this->CoefficientsC.WritePortal();
  auto D = this->CoefficientsD.WritePortal();

  C.Set(n - 1, 0.0);
  auto yVals = this->Values.ReadPortal();

  for (vtkm::Id i = n - 2; i >= 0; i--)
  {
    //c1D[j] = z[j] - mu[j] * c1D[j + 1];
    C.Set(i, z.Get(i) - mu.Get(i) * C.Get(i + 1));
    //b1D[j] = (a1D[j + 1] - a1D[j]) / h[j] - h[j] * (c1D[j + 1] + 2.0 * c1D[j]) / 3.0;
    auto val0 = (yVals.Get(i + 1) - yVals.Get(i)) / h.Get(i);
    auto val1 = h.Get(i) * (C.Get(i + 1) + 2.0 * C.Get(i)) / 3.0;
    B.Set(i, val0 - val1);
    //d1D[j] = (c1D[j + 1] - c1D[j]) / (3.0 * h[j]);
    D.Set(i, (C.Get(i + 1) - C.Get(i)) / (3.0 * h.Get(i)));
  }
}


void CubicSpline::Build()
{
  //Calculate the spline coeficients.
  vtkm::Id n = this->ControlPoints.GetNumberOfValues();
  if (n < 2)
    throw std::invalid_argument("At least two points are required for spline interpolation.");

  auto h = this->CalcH();
  auto alpha = this->CalcAlpha(h);

  this->CalcCoefficients(h, alpha);
}

vtkm::exec::CubicSpline CubicSpline::PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                         vtkm::cont::Token& token) const
{
  this->Update();

  using ExecObjType = vtkm::exec::CubicSpline;

  return ExecObjType(this->GetControlPoints(),
                     this->GetValues(),
                     this->CoefficientsB,
                     this->CoefficientsC,
                     this->CoefficientsD,
                     device,
                     token);
}

}
} // namespace vtkm::cont
