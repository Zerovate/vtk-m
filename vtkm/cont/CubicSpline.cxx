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
namespace
{
class CalculateH : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut h, WholeArrayIn xVals);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  template <typename ResultType, typename InputType>
  VTKM_EXEC void operator()(const vtkm::Id& idx, ResultType& h, const InputType& xVals) const
  {
    //h[i] = x1D[i + 1] - x1D[i];
    h = xVals.Get(idx + 1) - xVals.Get(idx);
  }
};

class CalculateAlpha : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut alpha, WholeArrayIn h, WholeArrayIn yVals);
  using ExecutionSignature = void(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  template <typename ResultType, typename HArrayType, typename YArrayType>
  VTKM_EXEC void operator()(const vtkm::Id& idx,
                            ResultType& alpha,
                            const HArrayType& h,
                            const YArrayType& y) const
  {
    //alpha[i - 1] = (3.0 * (a1D[i + 1] - a1D[i]) / h[i]) - (3.0 * (a1D[i] - a1D[i - 1]) / h[i - 1]);
    // alpha: i-1 --> i.  Add +1 to each i.
    //alpha[i] = (3.0 * (a1D[i + 2] - a1D[i+1]) / h[i+1]) - (3.0 * (a1D[i+1] - a1D[i]) / h[i]);

    alpha = (3.0 * (y.Get(idx + 2) - y.Get(idx + 1)) / h.Get(idx + 1)) -
      (3.0 * (y.Get(idx + 1) - y.Get(idx)) / h.Get(idx));
  }
};

class CalculateLMuZ : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut L,
                                WholeArrayOut Mu,
                                WholeArrayOut Z,
                                WholeArrayIn xVals,
                                WholeArrayIn alpha,
                                WholeArrayIn h);
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  CalculateLMuZ(const vtkm::Id& size)
    : Size(size)
  {
  }

  template <typename LType,
            typename MuType,
            typename ZType,
            typename XType,
            typename AlphaType,
            typename HType>
  VTKM_EXEC void operator()(const vtkm::Id& idx,
                            LType& L,
                            MuType& Mu,
                            ZType& Z,
                            const XType& xVal,
                            const AlphaType& alpha,
                            const HType& H) const
  {
    if (idx == 0)
    {
      L.Set(idx, 1.0);
      Mu.Set(idx, 0.0);
      Z.Set(idx, 0.0);
    }
    else if (idx == this->Size - 1)
    {
      L.Set(idx, 1.0);
      Z.Set(idx, 1.0);
    }
    else
    {
      //l[i] = 2.0 * (x1D[i + 1] - x1D[i - 1]) - h[i - 1] * mu[i - 1];
      L.Set(idx, 2.0 * (xVal.Get(idx + 1) - xVal.Get(idx - 1)) - H.Get(idx - 1) * Mu.Get(idx - 1));

      //mu[i] = h[i] / l[i];
      Mu.Set(idx, H.Get(idx) / L.Get(idx));

      //z[i] = (alpha[i - 1] - h[i - 1] * z[i - 1]) / l[i];
      Z.Set(idx, alpha.Get(idx - 1) - H.Get(idx - 1) * Z.Get(idx - 1) / L.Get(idx));
    }
  }

private:
  vtkm::Id Size;
};

} //namespace

template <typename ArrayType>
void CubicSpline::CalculateLMuZ(ArrayType& mu,
                                ArrayType& z,
                                const ArrayType& alpha,
                                const ArrayType& h) const
{
  auto Mu = mu.WritePortal();
  auto Z = z.WritePortal();
  auto Alpha = alpha.ReadPortal();
  const auto H = h.ReadPortal();
  const auto xVals = this->ControlPoints.ReadPortal();
  vtkm::Id size = h.GetNumberOfValues();

  // initial values.
  //L.Set(0, 1.0);
  Mu.Set(0, 0.0);
  Z.Set(0, 0.0);

  for (vtkm::Id i = 1; i < size - 1; i++)
  {
    auto L = 2.0 * (xVals.Get(i + 1) - xVals.Get(i - 1)) - H.Get(i - 1) * Mu.Get(i - 1);
    Mu.Set(i, H.Get(i) / L);
    Z.Set(i, (Alpha.Get(i - 1) - H.Get(i - 1) * Z.Get(i - 1)) / L);
  }
  //L.Set(size - 1, 1.0);
  Z.Set(size - 1, 0.0);
}

template <typename ArrayPortalType>
void CubicSpline::CalculateCoefficients(vtkm::Id n,
                                        const ArrayPortalType& H,
                                        const ArrayPortalType& Mu,
                                        const ArrayPortalType& Z)
{
  this->CoefficientsB.Allocate(n - 1);
  this->CoefficientsC.Allocate(n);
  this->CoefficientsD.Allocate(n - 1);
  auto B = this->CoefficientsB.WritePortal();
  auto C = this->CoefficientsC.WritePortal();
  auto D = this->CoefficientsD.WritePortal();
  const auto yVals = this->Values.ReadPortal();

  C.Set(n - 1, 0.0);

  for (vtkm::Id i = n - 2; i >= 0; i--)
  {
    //c1D[j] = z[j] - mu[j] * c1D[j + 1];
    auto val = Z.Get(i) - Mu.Get(i) * C.Get(i + 1);
    C.Set(i, val);

    //b1D[j] = (a1D[j + 1] - a1D[j]) / h[j] - h[j] * (c1D[j + 1] + 2.0 * c1D[j]) / 3.0;
    val = (yVals.Get(i + 1) - yVals.Get(i)) / H.Get(i) -
      H.Get(i) * (C.Get(i + 1) + 2.0 * C.Get(i)) / 3.0;
    B.Set(i, val);

    //d1D[j] = (c1D[j + 1] - c1D[j]) / (3.0 * h[j]);
    val = (C.Get(i + 1) - C.Get(i)) / (3.0 * H.Get(i));
    D.Set(i, val);
  }
}

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

  //calculate h
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> h;
  h.Allocate(n - 1);
  vtkm::cont::Invoker invoker;
  invoker(CalculateH{}, h, this->ControlPoints);

  //alpha[i - 1] = (3.0 * (a1D[i + 1] - a1D[i]) / h[i]) - (3.0 * (a1D[i] - a1D[i - 1]) / h[i - 1]);
  // for loop from 0 to n-2
  // alpha[i] = (3.0 * (a1D[i + 1] - a1D[i]) / h[i + 1]) - (3.0 * (a1D[i] - a1D[i - 1]) / h[i]);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> alpha;
  alpha.Allocate(n - 2);
  invoker(CalculateAlpha{}, alpha, h, this->Values);

  //calculate mu, z.
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> mu, z;
  mu.Allocate(n);
  z.Allocate(n);
  this->CalculateLMuZ(mu, z, alpha, h);
  this->CalculateCoefficients(n, h.ReadPortal(), mu.ReadPortal(), z.ReadPortal());
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
