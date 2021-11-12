//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_DispatcherPointNeighborhood_h
#define vtk_m_worklet_DispatcherPointNeighborhood_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{
class WorkletNeighborhood;
class WorkletPointNeighborhood;

/// \brief Dispatcher for worklets that inherit from \c WorkletPointNeighborhood.
///
template <typename WorkletType>
class DispatcherPointNeighborhood
  : public vtkm::worklet::internal::DispatcherBase<DispatcherPointNeighborhood<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletNeighborhood>
{
  using Superclass =
    vtkm::worklet::internal::DispatcherBase<DispatcherPointNeighborhood<WorkletType>,
                                            WorkletType,
                                            vtkm::worklet::WorkletNeighborhood>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherPointNeighborhood(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename... Args>
  void DoInvoke(Args&&... args) const
  {
    using namespace vtkm::worklet::internal; // For SchedulingRange

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the args using the superclass.
    const auto& inputDomain = this->GetInputDomain(std::forward<Args>(args)...);

    // If you get a compile error on this line, then you have tried to use
    // something that is not a vtkm::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VTKM_IS_CELL_SET(vtkm::internal::remove_pointer_and_decay<decltype(inputDomain)>);

    auto inputRange = SchedulingRange(inputDomain, vtkm::TopologyElementTagPoint{});

    // This is pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(inputRange, std::forward<Args>(args)...);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_DispatcherPointNeighborhood_h
