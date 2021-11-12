//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Dispatcher_MapTopology_h
#define vtk_m_worklet_Dispatcher_MapTopology_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{
namespace detail
{
struct WorkletMapTopologyBase;
}
class WorkletVisitCellsWithPoints;
class WorkletVisitPointsWithCells;

/// \brief Dispatcher for worklets that inherit from \c WorkletMapTopology.
///
template <typename WorkletType>
class DispatcherMapTopology
  : public vtkm::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::detail::WorkletMapTopologyBase>
{
  using Superclass =
    vtkm::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType>,
                                            WorkletType,
                                            vtkm::worklet::detail::WorkletMapTopologyBase>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherMapTopology(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename... Args>
  VTKM_CONT void DoInvoke(Args&&... args) const
  {
    using namespace vtkm::worklet::internal; // For SchedulingRange

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the args using the superclass.
    const auto& inputDomain = this->GetInputDomain(std::forward<Args>(args)...);

    // If you get a compile error on this line, then you have tried to use
    // something that is not a vtkm::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VTKM_IS_CELL_SET(vtkm::internal::remove_pointer_and_decay<decltype(inputDomain)>);

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BadicInvoke.
    this->BasicInvoke(SchedulingRange(inputDomain, typename WorkletType::VisitTopologyType{}),
                      std::forward<Args>(args)...);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapTopology_h
