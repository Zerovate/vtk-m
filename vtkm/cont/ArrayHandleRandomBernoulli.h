//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_count_ArrayHandleRandomBernoulli_h
#define vtk_m_count_ArrayHandleRandomBernoulli_h

#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/ArrayHandleTransform.h>

namespace vtkm
{
namespace cont
{
namespace detail
{

template <typename T, typename Flag>
struct InverseTransformBernoulli
{
  VTKM_EXEC_CONT
  InverseTransformBernoulli(T p = T(0.5))
    : P(p)
  {
  }

  // Discrete Inverse Transform Sampling for Bernoulli Distribution
  // Assume X to be a Bernoulli random variable with support {0,1}, then it's cdf it given by
  // F(x) = P(X <= x) =
  //		0 if x<0,
  //		1-p if 0<=x<1,
  //		1 if x>=1
  // The inverse cdf is F^-1(u) =
  //		0 if u <= 1-p
  //		1 if u > 1-p
  // With the discrete inverse transform property, we can transform samples from U(0,1) to have cdf F,
  // by applying F^-1
  VTKM_EXEC_CONT Flag operator()(const T x) const
  {
    return static_cast<Flag>(x > T(1.0) - this->P);
  }

  T P;
};
} //detail

template <typename Real = vtkm::Float64, typename Flag = vtkm::UInt8>
class VTKM_ALWAYS_EXPORT ArrayHandleRandomBernoulli
  : public vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleRandomUniformReal<Real>,
                                            detail::InverseTransformBernoulli<Real, Flag>>
{
public:
  using SeedType = vtkm::Vec<vtkm::UInt32, 1>;
  using UniformReal = vtkm::cont::ArrayHandleRandomUniformReal<Real>;

  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRandomBernoulli,
    (ArrayHandleRandomBernoulli<Real>),
    (vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleRandomUniformReal<Real>,
                                      detail::InverseTransformBernoulli<Real, Flag>>));

  explicit ArrayHandleRandomBernoulli(vtkm::Id length,
                                      Real p,
                                      SeedType seed = { std::random_device{}() })
    : Superclass(UniformReal{ length, seed }, detail::InverseTransformBernoulli<Real, Flag>{ p })
  {
  }
};
}
}
#endif // vtk_m_count_ArrayHandleRandomBernoulli_h
