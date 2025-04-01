//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_ArrayHandleIsMonotonic_hxx
#define vtk_m_cont_ArrayHandleIsMonotonic_hxx

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{

namespace
{
struct MonotonicIncreasing : public vtkm::worklet::WorkletMapField
{
  MonotonicIncreasing(const vtkm::Id numPoints)
    : NumPoints(numPoints)
  {
  }

  using ControlSignature = void(WholeArrayIn, FieldOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VTKM_EXEC void operator()(const vtkm::Id& idx, const ArrayType& input, bool& result) const
  {
    if (idx == 0)
      result = true;
    else
      result = input.Get(idx) >= input.Get(idx - 1);
  }

  vtkm::Id NumPoints;
};

struct MonotonicDecreasing : public vtkm::worklet::WorkletMapField
{
  MonotonicDecreasing(const vtkm::Id numPoints)
    : NumPoints(numPoints)
  {
  }

  using ControlSignature = void(WholeArrayIn, FieldOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VTKM_EXEC void operator()(const vtkm::Id& idx, const ArrayType& input, bool& result) const
  {
    if (idx == 0)
      result = true;
    else
      result = input.Get(idx) <= input.Get(idx - 1);
  }

  vtkm::Id NumPoints;
};
} //anonymous namespace


template <typename T>
VTKM_ALWAYS_EXPORT inline bool IsMonotonicIncreasing(
  const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& input)
{
  // TODO or false for empty array?
  vtkm::Id numValues = input.GetNumberOfValues();
  if (numValues < 2)
    return true;

  vtkm::cont::Invoker invoke;

  vtkm::cont::ArrayHandle<bool> result;
  invoke(MonotonicIncreasing(input.GetNumberOfValues()), input, result);
  return vtkm::cont::Algorithm::Reduce(result, true, vtkm::LogicalAnd());
}

template <typename T>
VTKM_ALWAYS_EXPORT inline bool IsMonotonicDecreasing(
  const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& input)
{
  // TODO or false for empty array?
  vtkm::Id numValues = input.GetNumberOfValues();
  if (numValues < 2)
    return true;

  vtkm::cont::Invoker invoke;

  vtkm::cont::ArrayHandle<bool> result;
  invoke(MonotonicDecreasing(input.GetNumberOfValues()), input, result);
  return vtkm::cont::Algorithm::Reduce(result, true, vtkm::LogicalAnd());
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleIsMonotonic_hxx
