//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MaterialBase_h
#define vtk_m_rendering_MaterialBase_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace rendering
{
template <typename Derived>
class MaterialBase : public vtkm::cont::ExecutionObjectBase
{
public:
  Derived PrepareForExecution(vtkm::cont::DeviceAdapterId vtkmNotUsed(deviceId),
                              vtkm::cont::Token& vtkmNotUsed(token)) const
  {
    const Derived* self = reinterpret_cast<const Derived*>(this);
    return *self;
  }
};
} // namespace rendering
} //namespace vtkm

#endif //vtk_m_rendering_MaterialBase_h
