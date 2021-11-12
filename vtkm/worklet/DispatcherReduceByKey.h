//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_DispatcherReduceByKey_h
#define vtk_m_worklet_DispatcherReduceByKey_h

#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/arg/TypeCheckTagKeys.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{
class WorkletReduceByKey;

/// \brief Dispatcher for worklets that inherit from \c WorkletReduceByKey.
///
template <typename WorkletType>
class DispatcherReduceByKey
  : public vtkm::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletReduceByKey>
{
  using Superclass = vtkm::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType>,
                                                             WorkletType,
                                                             vtkm::worklet::WorkletReduceByKey>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherReduceByKey(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename... Args>
  void DoInvoke(Args&&... args) const
  {
    using namespace vtkm::worklet::internal; // For SchedulingRange

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the args using the superclass.
    const auto& inputDomain = this->GetInputDomain(args...);

    // This is the type for the input domain
    using InputDomainType = vtkm::internal::remove_pointer_and_decay<decltype(inputDomain)>;

    // If you get a compile error on this line, then you have tried to use
    // something other than vtkm::worklet::Keys as the input domain, which
    // is illegal.
    VTKM_STATIC_ASSERT_MSG(
      (vtkm::cont::arg::TypeCheck<vtkm::cont::arg::TypeCheckTagKeys, InputDomainType>::value),
      "Invalid input domain for WorkletReduceByKey.");

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BasicInvoke.
    this->BasicInvoke(SchedulingRange(inputDomain), std::forward<Args>(args)...);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_DispatcherReduceByKey_h
