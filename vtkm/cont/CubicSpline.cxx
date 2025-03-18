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

void CubicSpline::Build()
{
  //TODO: convert this a worklet.


  //Calculate the spline coeficients.
  vtkm::Id n = this->ControlPoints.GetNumberOfValues();
  if (n < 2)
    throw std::invalid_argument("At least two points are required for spline interpolation.");

  // Allocate memory for spline coefficients
  this->CoefficientsB.Allocate(n - 1);
  this->CoefficientsC.Allocate(n);
  this->CoefficientsD.Allocate(n - 1);
  //a1D = y1D;
  //b1D.resize(n - 1);
  //c1D.resize(n);
  //d1D.resize(n - 1);

  //std::vector<double> h(n - 1), alpha(n - 2); // Fix: correct alpha size
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> h, alpha;
  h.Allocate(n - 1);
  alpha.Allocate(n - 2);


  for (vtkm::Id i = 0; i < n - 1; i++)
  {
    //h[i] = x1D[i + 1] - x1D[i];
    h.WritePortal().Set(
      i, this->ControlPoints.ReadPortal().Get(i + 1) - this->ControlPoints.ReadPortal().Get(i));
  }

  for (vtkm::Id i = 1; i < n - 1; ++i)
  {
    //alpha[i - 1] = (3.0 * (a1D[i + 1] - a1D[i]) / h[i]) - (3.0 * (a1D[i] - a1D[i - 1]) / h[i - 1]);
    alpha.WritePortal().Set(
      i - 1,
      (3.0 * (this->Values.ReadPortal().Get(i + 1) - this->Values.ReadPortal().Get(i)) /
       h.ReadPortal().Get(i)) -
        (3.0 * (this->Values.ReadPortal().Get(i) - this->Values.ReadPortal().Get(i - 1)) /
         h.ReadPortal().Get(i - 1)));
  }

  //std::vector<double> l(n), mu(n), z(n);
  //l[0] = 1.0;
  //mu[0] = z[0] = 0.0;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> l, mu, z;
  l.Allocate(n);
  mu.Allocate(n);
  z.Allocate(n);
  l.WritePortal().Set(0, 1.0);
  mu.WritePortal().Set(0, 0.0);
  z.WritePortal().Set(0, 0.0);


  for (vtkm::Id i = 1; i < n - 1; ++i)
  {
    //l[i] = 2.0 * (x1D[i + 1] - x1D[i - 1]) - h[i - 1] * mu[i - 1];
    l.WritePortal().Set(i,
                        2.0 *
                            (this->ControlPoints.ReadPortal().Get(i + 1) -
                             this->ControlPoints.ReadPortal().Get(i - 1)) -
                          h.ReadPortal().Get(i - 1) * mu.ReadPortal().Get(i - 1));
    //mu[i] = h[i] / l[i];
    mu.WritePortal().Set(i, h.ReadPortal().Get(i) / l.ReadPortal().Get(i));
    //z[i] = (alpha[i - 1] - h[i - 1] * z[i - 1]) / l[i];
    z.WritePortal().Set(
      i,
      (alpha.ReadPortal().Get(i - 1) - h.ReadPortal().Get(i - 1) * z.ReadPortal().Get(i - 1)) /
        l.ReadPortal().Get(i));
  }

  //l[n - 1] = 1.0;
  //z[n - 1] = c1D[n - 1] = 0.0;
  l.WritePortal().Set(n - 1, 1.0);
  z.WritePortal().Set(n - 1, 0.0);
  this->CoefficientsC.WritePortal().Set(n - 1, 0.0);

  for (vtkm::Id j = n - 2; j >= 0; j--)
  {
    //c1D[j] = z[j] - mu[j] * c1D[j + 1];
    this->CoefficientsC.WritePortal().Set(
      j,
      z.ReadPortal().Get(j) - mu.ReadPortal().Get(j) * this->CoefficientsC.ReadPortal().Get(j + 1));
    //b1D[j] = (a1D[j + 1] - a1D[j]) / h[j] - h[j] * (c1D[j + 1] + 2.0 * c1D[j]) / 3.0;
    auto val0 = (this->Values.ReadPortal().Get(j + 1) - this->Values.ReadPortal().Get(j)) /
      h.ReadPortal().Get(j);
    auto val1 = h.ReadPortal().Get(j) *
      (this->CoefficientsC.ReadPortal().Get(j + 1) +
       2.0 * this->CoefficientsC.ReadPortal().Get(j)) /
      3.0;
    this->CoefficientsB.WritePortal().Set(j, val0 - val1);
    //d1D[j] = (c1D[j + 1] - c1D[j]) / (3.0 * h[j]);
    this->CoefficientsD.WritePortal().Set(
      j,
      (this->CoefficientsC.ReadPortal().Get(j + 1) - this->CoefficientsC.ReadPortal().Get(j)) /
        (3.0 * h.ReadPortal().Get(j)));
  }
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
