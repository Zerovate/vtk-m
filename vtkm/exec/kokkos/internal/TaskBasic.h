//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_kokkos_internal_TaskBasic_h
#define vtk_m_exec_kokkos_internal_TaskBasic_h

#include <vtkm/exec/TaskBase.h>

#include <vtkm/Tuple.h>

#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

namespace vtkm
{
namespace exec
{
namespace kokkos
{
namespace internal
{

template <typename WorkletType,
          typename OutToInPortalType,
          typename VisitPortalType,
          typename ThreadToOutPortalType,
          typename Device,
          typename... ExecutionObjectTypes>
class VTKM_NEVER_EXPORT TaskBasic1DWorklet : public vtkm::exec::TaskBase
{
  std::decay_t<WorkletType> Worklet;
  std::decay_t<OutToInPortalType> OutToInPortal;
  std::decay_t<VisitPortalType> VisitPortal;
  std::decay_t<ThreadToOutPortalType> ThreadToOutPortal;
  vtkm::Tuple<std::decay_t<ExecutionObjectTypes>...> ExecutionObjects;

  inline auto GetInputDomain() const
  {
    // Note: Worklet parameter tags (such as _1, _2, _3), as is used for `InputDomain`
    // start their indexing at 1 whereas `Tuple` starts its indexing at 0.
    return vtkm::Get<WorkletType::InputDomain::INDEX - 1>(this->ExecutionObjects);
  }

public:
  TaskBasic1DWorklet(
      const WorkletType& worklet,
      const OutToInPortalType& outToInPortal,
      const VisitPortalType& visitPortal,
      const ThreadToOutPortalType& threadToOutPortal,
      const ExecutionObjectTypes&&... executionObjects)
    : Worklet(worklet)
    , OutToInPortal(outToInPortal)
    , VisitPortal(visitPortal)
    , ThreadToOutPortal(threadToOutPortal)
    , ExecutionObjects(std::forward<ExecutionObjectTypes>(executionObjects)...)
  {
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    vtkm::exec::internal::WorkletInvokeFunctor(
          this->Worklet,
          this->Worklet.GetThreadIndices(
            index,
            this->OutToInPortal,
            this->VisitPortal,
            this->ThreadToOutPortal,
            this->GetInputDomain()),
          Device{},
          this->ExecutionObjects);
  }
};

template <typename FunctorType>
class VTKM_NEVER_EXPORT TaskBasic1DFunctor : public vtkm::exec::TaskBase
{
  typename std::remove_const<FunctorType>::type Functor;

public:
  explicit TaskBasic1DFunctor(const FunctorType& functor)
    : Functor(functor)
  {
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Functor.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id index) const { this->Functor(index); }
};

template <typename WorkletType,
          typename OutToInPortalType,
          typename VisitPortalType,
          typename ThreadToOutPortalType,
          typename Device,
          typename... ExecutionObjectTypes>
class VTKM_NEVER_EXPORT TaskBasic3DWorklet : public vtkm::exec::TaskBase
{
  std::decay_t<WorkletType> Worklet;
  std::decay_t<OutToInPortalType> OutToInPortal;
  std::decay_t<VisitPortalType> VisitPortal;
  std::decay_t<ThreadToOutPortalType> ThreadToOutPortal;
  vtkm::Tuple<std::decay_t<ExecutionObjectTypes>...> ExecutionObjects;

  inline auto GetInputDomain() const
  {
    // Note: Worklet parameter tags (such as _1, _2, _3), as is used for `InputDomain`
    // start their indexing at 1 whereas `Tuple` starts its indexing at 0.
    return vtkm::Get<WorkletType::InputDomain::INDEX - 1>(this->ExecutionObjects);
  }

public:
  TaskBasic3DWorklet(
      const WorkletType& worklet,
      const OutToInPortalType& outToInPortal,
      const VisitPortalType& visitPortal,
      const ThreadToOutPortalType& threadToOutPortal,
      ExecutionObjectTypes&&... executionObjects)
    : Worklet(worklet)
    , OutToInPortal(outToInPortal)
    , VisitPortal(visitPortal)
    , ThreadToOutPortal(threadToOutPortal)
    , ExecutionObjects(std::forward<ExecutionObjectTypes>(executionObjects)...)
  {
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id3 index, vtkm::Id flatIndex) const
  {
    vtkm::exec::internal::WorkletInvokeFunctor(
          this->Worklet,
          this->Worklet.GetThreadIndices(
            flatIndex,
            index,
            this->OutToInPortal,
            this->VisitPortal,
            this->ThreadToOutPortal,
            this->GetInputDomain()),
          Device{},
          this->ExecutionObjects);
  }
};

template <typename FunctorType>
class VTKM_NEVER_EXPORT TaskBasic3DFunctor : public vtkm::exec::TaskBase
{
  typename std::remove_const<FunctorType>::type Functor;

public:
  explicit TaskBasic3DFunctor(const FunctorType& functor)
    : Functor(functor)
  {
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Functor.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id3 idx, vtkm::Id) const { this->Functor(idx); }
};

}
}
}
} // vtkm::exec::kokkos::internal

#endif //vtk_m_exec_kokkos_internal_TaskBasic_h
