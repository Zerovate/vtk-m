//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleIsMonotonic_h
#define vtk_m_cont_ArrayHandleIsMonotonic_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

template <typename T>
VTKM_ALWAYS_EXPORT inline bool IsMonotonicIncreasing(
  const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& input);

template <typename T>
VTKM_ALWAYS_EXPORT inline bool IsMonotonicDecreasing(
  const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& input);

}
} // namespace vtkm::cont

#ifndef vtk_m_cont_ArrayHandleIsMonotonic_hxx
#include <vtkm/cont/ArrayHandleIsMonotonic.hxx>
#endif //vtk_m_cont_ArrayHandleIsMonotonic_hxx

#endif //vtk_m_cont_ArrayHandleIsMonotonic_h
