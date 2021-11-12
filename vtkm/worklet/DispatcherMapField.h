//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Dispatcher_MapField_h
#define vtk_m_worklet_Dispatcher_MapField_h

#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{

class WorkletMapField;

/// \brief Dispatcher for worklets that inherit from \c WorkletMapField.
///
template <typename WorkletType>
class DispatcherMapField
  : public vtkm::worklet::internal::
      DispatcherBase<DispatcherMapField<WorkletType>, WorkletType, vtkm::worklet::WorkletMapField>
{
  using Superclass = vtkm::worklet::internal::
    DispatcherBase<DispatcherMapField<WorkletType>, WorkletType, vtkm::worklet::WorkletMapField>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherMapField(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename... Args>
  VTKM_CONT void DoInvoke(Args&&... args) const
  {
    // Get the number of instances by querying the scheduling range of the input domain
    using namespace vtkm::worklet::internal;
    auto numInstances = SchedulingRange(this->GetInputDomain(std::forward<Args>(args)...));

    // A MapField is a pretty straightforward dispatch. Once we know the number
    // of instances, the superclass can take care of the rest.
    this->BasicInvoke(numInstances, std::forward<Args>(args)...);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapField_h
