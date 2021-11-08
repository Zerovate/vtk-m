//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_cuda_internal_TaskStrided_h
#define vtk_m_exec_cuda_internal_TaskStrided_h

#include <vtkm/exec/TaskBase.h>

#include <vtkm/cont/cuda/internal/CudaAllocator.h>

//Todo: rename this header to TaskInvokeWorkletDetail.h
#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

namespace vtkm
{
namespace exec
{
namespace cuda
{
namespace internal
{

template <typename WorkletType,
          typename OutToInPortalType,
          typename VisitPortalType,
          typename ThreadToOutPortalType,
          typename Device,
          typename... ExecutionObjectTypes>
class VTKM_NEVER_EXPORT TaskStrided1DWorklet : public vtkm::exec::TaskBase
{
  std::decay_t<WorkletType> Worklet;
  std::decay_t<OutToInPortalType> OutToInPortal;
  std::decay_t<VisitPortalType> VisitPortal;
  std::decay_t<ThreadToOutPortalType> ThreadToOutPortal;
  vtkm::Tuple<std::decay_t<ExecutionObjectTypes>...> ExecutionObjects;

  VTKM_EXEC inline auto GetInputDomain() const
  {
    // Note: Worklet parameter tags (such as _1, _2, _3), as is used for `InputDomain`
    // start their indexing at 1 whereas `Tuple` starts its indexing at 0.
    return vtkm::Get<WorkletType::InputDomain::INDEX - 1>(this->ExecutionObjects);
  }

public:
  TaskStrided1DWorklet(const WorkletType& worklet,
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

  // Used for logging kernel launches
  const std::type_info& GetFunctorTypeId() const { return typeid(std::decay_t<WorkletType>); }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id start, vtkm::Id end, vtkm::Id inc) const
  {
    for (vtkm::Id index = start; index < end; index += inc)
    {
      vtkm::exec::internal::WorkletInvokeFunctor(
        this->Worklet,
        this->Worklet.GetThreadIndices(index,
                                       this->OutToInPortal,
                                       this->VisitPortal,
                                       this->ThreadToOutPortal,
                                       this->GetInputDomain()),
        Device{},
        this->ExecutionObjects);
    }
  }
};

template <typename FunctorType>
class VTKM_NEVER_EXPORT TaskStrided1DFunctor : public vtkm::exec::TaskBase
{
  typename std::remove_const<FunctorType>::type Functor;

public:
  explicit TaskStrided1DFunctor(const FunctorType& functor)
    : Functor(functor)
  {
  }

  // Used for logging kernel launches
  const std::type_info& GetFunctorTypeId() const
  {
    return typeid(std::remove_const_t<FunctorType>);
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Functor.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id start, vtkm::Id end, vtkm::Id inc) const
  {
    for (vtkm::Id index = start; index < end; index += inc)
    {
      this->Functor(index);
    }
  }
};

template <typename WorkletType,
          typename OutToInPortalType,
          typename VisitPortalType,
          typename ThreadToOutPortalType,
          typename Device,
          typename... ExecutionObjectTypes>
class VTKM_NEVER_EXPORT TaskStrided3DWorklet : public vtkm::exec::TaskBase
{
  std::decay_t<WorkletType> Worklet;
  std::decay_t<OutToInPortalType> OutToInPortal;
  std::decay_t<VisitPortalType> VisitPortal;
  std::decay_t<ThreadToOutPortalType> ThreadToOutPortal;
  vtkm::Tuple<std::decay_t<ExecutionObjectTypes>...> ExecutionObjects;

  VTKM_EXEC inline auto GetInputDomain() const
  {
    // Note: Worklet parameter tags (such as _1, _2, _3), as is used for `InputDomain`
    // start their indexing at 1 whereas `Tuple` starts its indexing at 0.
    return vtkm::Get<WorkletType::InputDomain::INDEX - 1>(this->ExecutionObjects);
  }

public:
  TaskStrided3DWorklet(const WorkletType& worklet,
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

  // Used for logging kernel launches
  const std::type_info& GetFunctorTypeId() const { return typeid(std::decay_t<WorkletType>); }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(const vtkm::Id3& size,
                  vtkm::Id start,
                  vtkm::Id end,
                  vtkm::Id inc,
                  vtkm::Id j,
                  vtkm::Id k) const
  {
    vtkm::Id3 index(start, j, k);
    auto threadIndex1D = index[0] + size[0] * (index[1] + size[1] * index[2]);
    for (vtkm::Id i = start; i < end; i += inc, threadIndex1D += inc)
    {
      index[0] = i;
      vtkm::exec::internal::WorkletInvokeFunctor(
        this->Worklet,
        this->Worklet.GetThreadIndices(threadIndex1D,
                                       index,
                                       this->OutToInPortal,
                                       this->VisitPortal,
                                       this->ThreadToOutPortal,
                                       this->GetInputDomain()),
        Device{},
        this->ExecutionObjects);
    }
  }
};

template <typename FunctorType>
class VTKM_NEVER_EXPORT TaskStrided3DFunctor : public vtkm::exec::TaskBase
{
  typename std::remove_const<FunctorType>::type Functor;

public:
  explicit TaskStrided3DFunctor(const FunctorType& functor)
    : Functor(functor)
  {
  }

  // Used for logging kernel launches
  const std::type_info& GetFunctorTypeId() const
  {
    return typeid(std::remove_const_t<FunctorType>);
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Functor.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(const vtkm::Id3& vtkmNotUsed(size),
                  vtkm::Id start,
                  vtkm::Id end,
                  vtkm::Id inc,
                  vtkm::Id j,
                  vtkm::Id k) const
  {
    vtkm::Id3 index(start, j, k);
    for (vtkm::Id i = start; i < end; i += inc)
    {
      index[0] = i;
      this->Functor(index);
    }
  }
};

}
}
}
} // vtkm::exec::cuda::internal

#endif //vtk_m_exec_cuda_internal_TaskStrided_h
