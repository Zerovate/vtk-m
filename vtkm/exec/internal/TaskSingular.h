//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_internal_TaskSingular_h
#define vtk_m_exec_internal_TaskSingular_h

#include <vtkm/Tuple.h>

#include <vtkm/exec/TaskBase.h>

#include <vtkm/exec/arg/Fetch.h>

#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

namespace vtkm
{
namespace exec
{
namespace internal
{

// TaskSingular represents an execution pattern for a worklet
// that is best expressed in terms of single dimension iteration space. Inside
// this single dimension no order is preferred.
//
//
template <typename Device,
          typename WorkletType,
          typename OutToInPortalType,
          typename VisitPortalType,
          typename ThreadToOutPortalType,
          typename InputDomainType,
          typename... ExecutionObjectTypes>
class VTKM_NEVER_EXPORT TaskSingular : public vtkm::exec::TaskBase
{
public:
  template <typename... EOTs>
  VTKM_CONT
  TaskSingular(const WorkletType& worklet,
               OutToInPortalType outToInPortal,
               VisitPortalType visitPortal,
               ThreadToOutPortalType threadToOutPortal,
               InputDomainType inputDomain,
               EOTs&&... executionObjects)
    : Worklet(worklet)
    , OutToInPortal(outToInPortal)
    , VisitPortal(visitPortal)
    , ThreadToOutPortal(threadToOutPortal)
    , InputDomain(inputDomain)
    , ExecutionObjects(std::forward<EOTs>(executionObjects)...)
  {
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VTKM_EXEC void operator()(T index) const
  {
    vtkm::exec::internal::WorkletInvokeFunctor(
      this->Worklet,
      this->Worklet.GetThreadIndices(index,
                                     this->OutToInPortal,
                                     this->VisitPortal,
                                     this->ThreadToOutPortal,
                                     this->InputDomain),
      Device{},
      this->ExecutionObjects);
  }

private:
  typename std::remove_const<WorkletType>::type Worklet;
  OutToInPortalType OutToInPortal;
  VisitPortalType VisitPortal;
  ThreadToOutPortalType ThreadToOutPortal;
  InputDomainType InputDomain;
  // This is held by by value so that when we transfer the execution objects
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const vtkm::Tuple<std::decay_t<ExecutionObjectTypes>...> ExecutionObjects;
};
}
}
} // vtkm::exec::internal

#endif //vtk_m_exec_internal_TaskSingular_h
