//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_exec_serial_internal_TaskTiling_h
#define vtk_m_exec_serial_internal_TaskTiling_h

#include <vtkm/exec/TaskBase.h>

#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

namespace vtkm
{
namespace exec
{
namespace serial
{
namespace internal
{

template <typename WorkletType,
          typename OutToInPortalType,
          typename VisitPortalType,
          typename ThreadToOutPortalType,
          typename Device,
          typename... ExecutionObjectTypes>
struct VTKM_NEVER_EXPORT WorkletInvokeInfo
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

  inline Device GetDevice() const { return Device{}; }
};

template <typename Functor>
struct VTKM_NEVER_EXPORT FunctorInvokeInfo
{
  Functor Worklet;
};

template <typename IType>
VTKM_NEVER_EXPORT void DeleteInvokeInfo(void* w)
{
  using InvokeInfoType = std::remove_cv_t<IType>;
  InvokeInfoType* const invokeInfo = static_cast<InvokeInfoType*>(w);
  delete invokeInfo;
}

template <typename WIType>
VTKM_NEVER_EXPORT void TaskTilingSetErrorBuffer(
  void* w,
  const vtkm::exec::internal::ErrorMessageBuffer& buffer)
{
  using WorkletInfoType = std::remove_cv_t<WIType>;
  WorkletInfoType* const workletInfo = static_cast<WorkletInfoType*>(w);
  workletInfo->Worklet.SetErrorMessageBuffer(buffer);
}

template <typename WIType>
VTKM_NEVER_EXPORT void TaskTiling1DExecute(void* w, vtkm::Id start, vtkm::Id end)
{
  using WorkletInfoType = std::remove_cv_t<WIType>;

  WorkletInfoType const* const workletInfo = static_cast<WorkletInfoType*>(w);

  for (vtkm::Id index = start; index < end; ++index)
  {
    vtkm::exec::internal::WorkletInvokeFunctor(
      workletInfo->Worklet,
      workletInfo->Worklet.GetThreadIndices(index,
                                            workletInfo->OutToInPortal,
                                            workletInfo->VisitPortal,
                                            workletInfo->ThreadToOutPortal,
                                            workletInfo->GetInputDomain()),
      workletInfo->GetDevice(),
      workletInfo->ExecutionObjects);
  }
}

template <typename FType>
VTKM_NEVER_EXPORT void FunctorTiling1DExecute(void* f, vtkm::Id start, vtkm::Id end)
{
  using FunctorType = typename std::remove_cv<FType>::type;
  FunctorInvokeInfo<FunctorType> const* const functorInfo =
    static_cast<FunctorInvokeInfo<FunctorType>*>(f);

  for (vtkm::Id index = start; index < end; ++index)
  {
    functorInfo->Worklet(index);
  }
}

template <typename WIType>
VTKM_NEVER_EXPORT void TaskTiling3DExecute(void* w,
                                           const vtkm::Id3& maxSize,
                                           vtkm::Id istart,
                                           vtkm::Id iend,
                                           vtkm::Id j,
                                           vtkm::Id k)
{
  using WorkletInfoType = std::remove_cv_t<WIType>;

  WorkletInfoType const* const workletInfo = static_cast<WorkletInfoType*>(w);

  vtkm::Id3 index(istart, j, k);
  auto threadIndex1D = index[0] + maxSize[0] * (index[1] + maxSize[1] * index[2]);
  for (vtkm::Id i = istart; i < iend; ++i, ++threadIndex1D)
  {
    index[0] = i;
    vtkm::exec::internal::WorkletInvokeFunctor(
      workletInfo->Worklet,
      workletInfo->Worklet.GetThreadIndices(threadIndex1D,
                                            index,
                                            workletInfo->OutToInPortal,
                                            workletInfo->VisitPortal,
                                            workletInfo->ThreadToOutPortal,
                                            workletInfo->GetInputDomain()),
      workletInfo->GetDevice(),
      workletInfo->ExecutionObjects);
  }
}

template <typename FType>
VTKM_NEVER_EXPORT void FunctorTiling3DExecute(void* f,
                                              const vtkm::Id3& vtkmNotUsed(maxSize),
                                              vtkm::Id istart,
                                              vtkm::Id iend,
                                              vtkm::Id j,
                                              vtkm::Id k)
{
  using FunctorType = typename std::remove_cv<FType>::type;
  FunctorInvokeInfo<FunctorType> const* const functorInfo =
    static_cast<FunctorInvokeInfo<FunctorType>*>(f);

  vtkm::Id3 index(istart, j, k);
  for (vtkm::Id i = istart; i < iend; ++i)
  {
    index[0] = i;
    functorInfo->Worklet(index);
  }
}

// TaskTiling1D represents an execution pattern for a worklet
// that is best expressed in terms of single dimension iteration space. TaskTiling1D
// also states that for best performance a linear consecutive range of values
// should be given to the worklet for optimal performance.
//
// Note: The worklet and invocation must have a lifetime that is at least
// as long as the Task
class VTKM_NEVER_EXPORT TaskTiling1D : public vtkm::exec::TaskBase
{
public:
  TaskTiling1D() = default;

  ~TaskTiling1D()
  {
    if (this->DeleteCallInfo != nullptr)
    {
      this->DeleteCallInfo(this->CallInfo);
      this->CallInfo = nullptr;
    }
    VTKM_ASSERT(this->CallInfo == nullptr);
  }

  /// This constructor supports general functors that which have a call
  /// operator with the signature:
  ///   operator()(vtkm::Id)
  template <typename FunctorType>
  TaskTiling1D(FunctorType& functor)
  {
    using FunctorInfoType = FunctorInvokeInfo<FunctorType>;

    //Setup the execute and set error buffer function pointers
    this->ExecuteFunction = &FunctorTiling1DExecute<FunctorType>;
    this->SetErrorBufferFunction = &TaskTilingSetErrorBuffer<FunctorInfoType>;

    this->CallInfo = new FunctorInfoType{ functor };
    this->DeleteCallInfo = DeleteInvokeInfo<FunctorInfoType>;
  }

  /// This constructor supports any vtkm worklet and the associated invocation
  /// parameters that go along with it
  template <typename WorkletType,
            typename OutToInPortalType,
            typename VisitPortalType,
            typename ThreadToOutPortalType,
            typename Device,
            typename... ExecutionObjectTypes>
  TaskTiling1D(const WorkletType& worklet,
               const OutToInPortalType& outToInPortal,
               const VisitPortalType& visitPortal,
               const ThreadToOutPortalType& threadToOutPortal,
               Device,
               ExecutionObjectTypes&&... executionObjects)
  {
    using WorkletInfoType = WorkletInvokeInfo<WorkletType,
                                              OutToInPortalType,
                                              VisitPortalType,
                                              ThreadToOutPortalType,
                                              Device,
                                              std::decay_t<ExecutionObjectTypes>...>;

    this->CallInfo = new WorkletInfoType{ worklet,
                                          outToInPortal,
                                          visitPortal,
                                          threadToOutPortal,
                                          vtkm::MakeTuple(std::forward<ExecutionObjectTypes>(
                                            executionObjects)...) };
    this->DeleteCallInfo = DeleteInvokeInfo<WorkletInfoType>;

    //Setup the execute and set error buffer function pointers
    this->ExecuteFunction = &TaskTiling1DExecute<WorkletInfoType>;
    this->SetErrorBufferFunction = &TaskTilingSetErrorBuffer<WorkletInfoType>;
  }

  TaskTiling1D(const TaskTiling1D&) = delete;

  TaskTiling1D(TaskTiling1D&& task) = default;

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->SetErrorBufferFunction(this->CallInfo, buffer);
  }

  void operator()(vtkm::Id start, vtkm::Id end) const
  {
    this->ExecuteFunction(this->CallInfo, start, end);
  }

protected:
  void* CallInfo = nullptr;

  using DeleteCallInfoSignature = void (*)(void*);
  DeleteCallInfoSignature DeleteCallInfo = nullptr;

  using ExecuteSignature = void (*)(void*, vtkm::Id, vtkm::Id);
  ExecuteSignature ExecuteFunction = nullptr;

  using SetErrorBufferSignature = void (*)(void*, const vtkm::exec::internal::ErrorMessageBuffer&);
  SetErrorBufferSignature SetErrorBufferFunction = nullptr;
};

// TaskTiling3D represents an execution pattern for a worklet
// that is best expressed in terms of an 3 dimensional iteration space. TaskTiling3D
// also states that for best performance a linear consecutive range of values
// in the X dimension should be given to the worklet for optimal performance.
//
// Note: The worklet and invocation must have a lifetime that is at least
// as long as the Task
class VTKM_NEVER_EXPORT TaskTiling3D : public vtkm::exec::TaskBase
{
public:
  TaskTiling3D() = default;

  ~TaskTiling3D()
  {
    if (this->DeleteCallInfo != nullptr)
    {
      this->DeleteCallInfo(this->CallInfo);
      this->CallInfo = nullptr;
    }
    VTKM_ASSERT(this->CallInfo == nullptr);
  }

  /// This constructor supports general functors that which have a call
  /// operator with the signature:
  ///   operator()(vtkm::Id)
  template <typename FunctorType>
  TaskTiling3D(FunctorType& functor)
  {
    using FunctorInfoType = FunctorInvokeInfo<FunctorType>;

    //Setup the execute and set error buffer function pointers
    this->ExecuteFunction = &FunctorTiling3DExecute<FunctorType>;
    this->SetErrorBufferFunction = &TaskTilingSetErrorBuffer<FunctorInfoType>;

    this->CallInfo = new FunctorInfoType{ functor };
    this->DeleteCallInfo = DeleteInvokeInfo<FunctorInfoType>;
  }

  template <typename WorkletType,
            typename OutToInPortalType,
            typename VisitPortalType,
            typename ThreadToOutPortalType,
            typename Device,
            typename... ExecutionObjectTypes>
  TaskTiling3D(const WorkletType& worklet,
               const OutToInPortalType& outToInPortal,
               const VisitPortalType& visitPortal,
               const ThreadToOutPortalType& threadToOutPortal,
               Device,
               ExecutionObjectTypes&&... executionObjects)
  {
    using WorkletInfoType = WorkletInvokeInfo<WorkletType,
                                              OutToInPortalType,
                                              VisitPortalType,
                                              ThreadToOutPortalType,
                                              Device,
                                              std::decay_t<ExecutionObjectTypes>...>;

    this->CallInfo = new WorkletInfoType{ worklet,
                                          outToInPortal,
                                          visitPortal,
                                          threadToOutPortal,
                                          vtkm::MakeTuple(std::forward<ExecutionObjectTypes>(
                                            executionObjects)...) };
    this->DeleteCallInfo = DeleteInvokeInfo<WorkletInfoType>;

    // Setup the execute and set error buffer function pointers
    this->ExecuteFunction = &TaskTiling3DExecute<WorkletInfoType>;
    this->SetErrorBufferFunction = &TaskTilingSetErrorBuffer<WorkletInfoType>;
  }

  TaskTiling3D(const TaskTiling3D& task) = delete;

  TaskTiling3D(TaskTiling3D&& task) = default;

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->SetErrorBufferFunction(this->CallInfo, buffer);
  }

  void operator()(const vtkm::Id3& maxSize,
                  vtkm::Id istart,
                  vtkm::Id iend,
                  vtkm::Id j,
                  vtkm::Id k) const
  {
    this->ExecuteFunction(this->CallInfo, maxSize, istart, iend, j, k);
  }

protected:
  void* CallInfo = nullptr;

  using DeleteCallInfoSignature = void (*)(void*);
  DeleteCallInfoSignature DeleteCallInfo = nullptr;

  using ExecuteSignature =
    void (*)(void*, const vtkm::Id3&, vtkm::Id, vtkm::Id, vtkm::Id, vtkm::Id);
  ExecuteSignature ExecuteFunction = nullptr;

  using SetErrorBufferSignature = void (*)(void*, const vtkm::exec::internal::ErrorMessageBuffer&);
  SetErrorBufferSignature SetErrorBufferFunction = nullptr;
};
}
}
}
} // vtkm::exec::serial::internal

#endif //vtk_m_exec_serial_internal_TaskTiling_h
