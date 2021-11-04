//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_internal_WorkletInvokeFunctor_h
#define vtk_m_exec_internal_WorkletInvokeFunctor_h

#include <vtkm/List.h>
#include <vtkm/Pair.h>
#include <vtkm/Tuple.h>

#include <vtkm/worklet/internal/Placeholders.h>

#include <vtkm/exec/arg/FetchTagExecObject.h>

// TODO: Delete this probably
#include <vtkmstd/integer_sequence.h>
// TODO: Definitely delete this
#include <vtkm/internal/FunctionInterface.h>

namespace vtkm
{
namespace exec
{
namespace internal
{

namespace detail
{

// Used to simulate an ControlSignature tag to go with the Device ExecutionSignature tag
struct ControlTagDeviceFaker
{
  using FetchTag = vtkm::exec::arg::FetchTagExecObject;
};

template <typename Worklet>
using ControlSignature = typename Worklet::ControlSignature;

template <typename Worklet>
using ExecutionSignature = typename vtkm::placeholders::GetExecSig<Worklet>::ExecutionSignature;

template <typename Signature>
struct VTKM_NEVER_EXPORT SignatureToTupleImpl;

template <typename R, typename... Args>
struct VTKM_NEVER_EXPORT SignatureToTupleImpl<R(Args...)>
{
  using type = vtkm::Tuple<Args...>;
};

template <typename Signature>
using SignatureToTuple = typename SignatureToTupleImpl<Signature>::type;

template <typename Signature>
struct VTKM_NEVER_EXPORT SignatureToReturnImpl;

template <typename R, typename... Args>
struct VTKM_NEVER_EXPORT SignatureToReturnImpl<R(Args...)>
{
  using type = R;
};

template <typename Signature>
using SignatureToReturn = typename SignatureToReturnImpl<Signature>::type;

template <typename ControlSig>
struct VTKM_NEVER_EXPORT ExtendedControlSignatureImpl;
template <typename R, typename... Args>
struct VTKM_NEVER_EXPORT ExtendedControlSignatureImpl<R(Args...)>
{
  using type = vtkm::List<detail::ControlTagDeviceFaker, Args...>;
};

template <typename Worklet>
using ExtendedControlSignatureToList =
  typename ExtendedControlSignatureImpl<ControlSignature<Worklet>>::type;

template <typename ExecSigReturn,
          typename ControlTagTuple,
          typename ThreadIndices,
          typename ExecObjectList>
struct VTKM_NEVER_EXPORT ExpectedWorkletReturnImpl
{
  using FetchTag = typename vtkm::TupleElement<ExecSigReturn::INDEX - 1, ControlTagTuple>::FetchTag;
  using AspectTag = typename ExecSigReturn::AspectTag;
  using ExecObjectType = std::decay_t<vtkm::ListAt<ExecObjectList, ExecSigReturn::INDEX - 1>>;
  using FetchType = vtkm::exec::arg::Fetch<FetchTag, AspectTag, ExecObjectType>;

  using type = std::decay_t<decltype(
    std::declval<FetchType>().Load(std::declval<ThreadIndices>(), std::declval<ExecObjectType>()))>;
};
template <typename ControlTagTuple, typename ThreadIndices, typename ExecObjectList>
struct VTKM_NEVER_EXPORT
  ExpectedWorkletReturnImpl<void, ControlTagTuple, ThreadIndices, ExecObjectList>
{
  using type = void;
};
template <typename WorkletType, typename ThreadIndices, typename... ExecObjects>
using ExpectedWorkletReturn =
  typename ExpectedWorkletReturnImpl<SignatureToReturn<ExecutionSignature<WorkletType>>,
                                     SignatureToTuple<ControlSignature<WorkletType>>,
                                     ThreadIndices,
                                     vtkm::List<ExecObjects...>>::type;

// TODO: Build a list of these with pyexpander
template <typename T0, typename... Rest>
VTKM_NEVER_EXPORT VTKM_EXEC inline T0&& GetArg(std::integral_constant<vtkm::IdComponent, 0>,
                                               T0&& arg,
                                               Rest&&...)
{
  return std::forward<T0>(arg);
}

template <vtkm::IdComponent I, typename T0, typename... Rest>
VTKM_NEVER_EXPORT VTKM_EXEC inline auto&& GetArg(std::integral_constant<vtkm::IdComponent, I>,
                                                 T0&&,
                                                 Rest&&... rest)
{
  VTKM_STATIC_ASSERT(I > 0);
  return GetArg(std::integral_constant<vtkm::IdComponent, I - 1>{}, std::forward<Rest>(rest)...);
}

template <typename WorkletType, typename ReturnType>
struct VTKM_NEVER_EXPORT CallWorklet
{
  // Note that this is a reference. Be mindful of scoping.
  const WorkletType& Worklet;
  ReturnType Result;

  VTKM_EXEC inline CallWorklet(const WorkletType& worklet)
    : Worklet(worklet)
  {
  }

  template <typename... TagAndWorkletArgs>
  VTKM_EXEC inline void operator()(TagAndWorkletArgs&&... tagAndWorkletArgs)
  {
    // If you got a compile error on the following line, it probably means that
    // the operator() of a worklet does not match the definition expected. One
    // common problem is that the operator() method must be declared const. Check
    // to make sure the "const" keyword is after parameters. Another common
    // problem is that the type of one or more parameters is incompatible with
    // the actual type that VTK-m creates in the execution environment. Make sure
    // that the types of the worklet operator() parameters match those in the
    // ExecutionSignature. The compiler error might help you narrow down which
    // parameter is wrong and the types that did not match.
    this->Result = this->Worklet(tagAndWorkletArgs.second...);
  }

  template <typename ThreadIndices, typename... ExecObjectTypes>
  VTKM_EXEC inline void StoreResult(const ThreadIndices& threadIndices,
                                    ExecObjectTypes&&... execObjects) const
  {
    using ExecTag = SignatureToReturn<ExecutionSignature<WorkletType>>;
    using ControlTagTuple = SignatureToTuple<ControlSignature<WorkletType>>;
    using FetchTag = typename vtkm::TupleElement<ExecTag::INDEX - 1, ControlTagTuple>::FetchTag;
    auto&& executionObject = GetArg(std::integral_constant<vtkm::IdComponent, ExecTag::INDEX - 1>{},
                                    std::forward<ExecObjectTypes>(execObjects)...);
    vtkm::exec::arg::
      Fetch<FetchTag, typename ExecTag::AspectTag, std::decay_t<decltype(executionObject)>>
        fetch;
    fetch.Store(threadIndices, executionObject, this->Result);
  }
};

template <typename WorkletType>
struct VTKM_NEVER_EXPORT CallWorklet<WorkletType, void>
{
  // Note that this is a reference. Be mindful of scoping.
  const WorkletType& Worklet;

  VTKM_EXEC inline CallWorklet(const WorkletType& worklet)
    : Worklet(worklet)
  {
  }

  template <typename... TagAndWorkletArgs>
  VTKM_EXEC inline void operator()(TagAndWorkletArgs&&... tagAndWorkletArgs) const
  {
    // If you got a compile error on the following line, it probably means that
    // the operator() of a worklet does not match the definition expected. One
    // common problem is that the operator() method must be declared const. Check
    // to make sure the "const" keyword is after parameters. Another common
    // problem is that the type of one or more parameters is incompatible with
    // the actual type that VTK-m creates in the execution environment. Make sure
    // that the types of the worklet operator() parameters match those in the
    // ExecutionSignature. The compiler error might help you narrow down which
    // parameter is wrong and the types that did not match.
    this->Worklet(tagAndWorkletArgs.second...);
  }

  template <typename ThreadIndices, typename... ExecObjectTypes>
  VTKM_EXEC inline void StoreResult(const ThreadIndices&, ExecObjectTypes&&...) const
  {
    // No return from worklet, so don't need to store the return.
  }
};

} // namespace detail

template <typename WorkletType,
          typename ThreadIndicesType,
          typename Device,
          typename... ExecObjTypes>
VTKM_NEVER_EXPORT VTKM_EXEC void WorkletInvokeFunctor(const WorkletType& worklet,
                                                      const ThreadIndicesType& threadIndices,
                                                      Device device,
                                                      ExecObjTypes&&... execObjects)
{
  using ExecutionSignature = detail::ExecutionSignature<WorkletType>;
  detail::SignatureToTuple<ExecutionSignature> executionTags;

  // All input arguments are references starting at index 1. Also, index 0 is specially
  // reserved to hold the DeviceAdapterTag. Handle both of these by extending the control
  // tags and execObjects by one.
  using ExtendedControlTagList = detail::ExtendedControlSignatureToList<WorkletType>;

  // Get the fetch object associated with each executionTag (which is in turn associated with
  // each worklet argument) and load the associated worklet argument. Return a `Tuple` containing
  // `Pair`s of each `ExecTag` and its associated worklet argument.
  auto tagAndWorkletArgs =
    executionTags.Transform([&](auto execTag) {
      using ExecTag = decltype(execTag);
      using ContTag = vtkm::ListAt<ExtendedControlTagList, ExecTag::INDEX>;
      using FetchTag = typename ContTag::FetchTag;
      auto&& execArg = detail::GetArg(std::integral_constant<vtkm::IdComponent, ExecTag::INDEX>{},
                                      device,
                                      std::forward<ExecObjTypes>(execObjects)...);
      vtkm::exec::arg::Fetch<FetchTag, typename ExecTag::AspectTag, std::decay_t<decltype(execArg)>>
        fetch;
      return vtkm::make_Pair(execTag, fetch.Load(threadIndices, execArg));
    });

  // Call the worklet with all the arguments (after pulling them out of the pairs).
  detail::CallWorklet<
    WorkletType,
    detail::ExpectedWorkletReturn<WorkletType, ThreadIndicesType, ExecObjTypes...>>
    callWorkletFunctor(worklet);
  tagAndWorkletArgs.Apply(callWorkletFunctor);

  // Store the arguments
  callWorkletFunctor.StoreResult(threadIndices, std::forward<ExecObjTypes>(execObjects)...);
  tagAndWorkletArgs.ForEach([&](auto& fetchAndArg) {
    using ExecTag = decltype(fetchAndArg.first);
    using ContTag = vtkm::ListAt<ExtendedControlTagList, ExecTag::INDEX>;
    using FetchTag = typename ContTag::FetchTag;
    auto&& execArg = detail::GetArg(std::integral_constant<vtkm::IdComponent, ExecTag::INDEX>{},
                                    device,
                                    std::forward<ExecObjTypes>(execObjects)...);
    vtkm::exec::arg::Fetch<FetchTag, typename ExecTag::AspectTag, std::decay_t<decltype(execArg)>>
      fetch;
    return fetch.Store(threadIndices, execArg, fetchAndArg.second);
  });
}

namespace detail
{

template <typename Worklet,
          typename Invocation,
          typename ThreadIndices,
          vtkm::IdComponent... Indices>
VTKM_EXEC void DoWorkletInvokeFunctor(const Worklet& worklet,
                                      const Invocation& invocation,
                                      const ThreadIndices& threadIndices,
                                      std::integer_sequence<vtkm::IdComponent, Indices...>)
{
  vtkm::exec::internal::WorkletInvokeFunctor(
    worklet,
    threadIndices,
    typename Invocation::DeviceAdapterTag{},
    vtkm::internal::ParameterGet<Indices + 1>(invocation.Parameters)...);
}

// TODO: Remove this function and replace with WorkletInvokeFunctor
template <typename Worklet, typename Invocation, typename ThreadIndices>
VTKM_EXEC void DoWorkletInvokeFunctor(const Worklet& worklet,
                                      const Invocation& invocation,
                                      const ThreadIndices& threadIndices)
{
  DoWorkletInvokeFunctor(
    worklet,
    invocation,
    threadIndices,
    typename vtkmstd::make_integer_sequence<vtkm::IdComponent,
                                            Invocation::ParameterInterface::ARITY>{});
}

} // namespace detail

} // namespace vtkm::exec::internal
} // namespace vtkm::exec
} // namespace vtkm

#endif //vtk_m_exec_internal_WorkletInvokeFunctor_h
