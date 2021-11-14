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

// The index in the executive object agument list starts at 1. Index 0 is reseved for the
// device adapter tag. GetAdjustedExecObj is an overloaded function that gets the correct
// object based on the index.
template <vtkm::IdComponent INDEX, typename ExecObjsTuple>
VTKM_NEVER_EXPORT VTKM_EXEC inline auto GetAdjustedExecObj(
  std::integral_constant<vtkm::IdComponent, INDEX>,
  const ExecObjsTuple& execObjsTuple)
{
  return vtkm::Get<INDEX - 1>(execObjsTuple);
}

template <vtkm::IdComponent INDEX, typename ExecObjsTuple, typename Device>
VTKM_NEVER_EXPORT VTKM_EXEC inline auto GetAdjustedExecObj(
  std::integral_constant<vtkm::IdComponent, INDEX>,
  const ExecObjsTuple& execObjsTuple,
  Device)
{
  return vtkm::Get<INDEX - 1>(execObjsTuple);
}

template <typename ExecObjsTuple, typename Device>
VTKM_NEVER_EXPORT VTKM_EXEC inline Device GetAdjustedExecObj(
  std::integral_constant<vtkm::IdComponent, 0>,
  const ExecObjsTuple&,
  Device device)
{
  return device;
}

template <typename ExecutionSignature>
struct VTKM_NEVER_EXPORT DoWorkletInvokeFunctor;

template <typename ExecSigRet, typename... ExecSigArgs>
struct VTKM_NEVER_EXPORT DoWorkletInvokeFunctor<ExecSigRet(ExecSigArgs...)>
{
  template <typename WorkletType,
            typename ThreadIndicesType,
            typename Device,
            typename ExecObjTuple>
  VTKM_NEVER_EXPORT static void Go(const WorkletType& worklet,
                                   const ThreadIndicesType& threadIndices,
                                   Device device,
                                   const ExecObjTuple& execObjectTuple)
  {
    // All input arguments are references starting at index 1. Also, index 0 is specially
    // reserved to hold the DeviceAdapterTag. Handle both of these by extending the control
    // tags and execObjects by one.
    using ExtendedControlTagList = detail::ExtendedControlSignatureToList<WorkletType>;

    // Get the fetch object associated with each executionTag (which is in turn associated with
    // each worklet argument) and load the associated worklet argument. Return a `Tuple` containing
    // `Pair`s of each `ExecTag` and its associated worklet argument.
    auto loadWorkletArg = [&](auto execTag) {
      using ExecTag = decltype(execTag);
      using ContTag = vtkm::ListAt<ExtendedControlTagList, ExecTag::INDEX>;
      using FetchTag = typename ContTag::FetchTag;
      auto&& execArg = detail::GetAdjustedExecObj(
        std::integral_constant<vtkm::IdComponent, ExecTag::INDEX>{}, execObjectTuple, device);
      vtkm::exec::arg::Fetch<FetchTag, typename ExecTag::AspectTag, std::decay_t<decltype(execArg)>>
        fetch;
      return vtkm::make_Pair(execTag, fetch.Load(threadIndices, execArg));
    };
    auto tagAndWorkletArgs = vtkm::MakeTuple(loadWorkletArg(ExecSigArgs{})...);

    // Call the worklet with the fetch arguments.
    auto callWorklet = [&](auto&&... workletArgPairs) {
      return worklet(workletArgPairs.second...);
    };
    auto result = tagAndWorkletArgs.Apply(callWorklet);

    // Store the result of the arguments.
    auto storeWorkletArg = [&](auto&& fetchAndArgPair) {
      using ExecTag = decltype(fetchAndArgPair.first);
      using ContTag = vtkm::ListAt<ExtendedControlTagList, ExecTag::INDEX>;
      using FetchTag = typename ContTag::FetchTag;
      auto&& execArg = detail::GetAdjustedExecObj(
        std::integral_constant<vtkm::IdComponent, ExecTag::INDEX>{}, execObjectTuple, device);
      vtkm::exec::arg::Fetch<FetchTag, typename ExecTag::AspectTag, std::decay_t<decltype(execArg)>>
        fetch;
      fetch.Store(threadIndices, execArg, fetchAndArgPair.second);
    };
    tagAndWorkletArgs.ForEach(storeWorkletArg);
    storeWorkletArg(vtkm::make_Pair(ExecSigRet{}, result));
  }
};

template <typename... ExecSigArgs>
struct VTKM_NEVER_EXPORT DoWorkletInvokeFunctor<void(ExecSigArgs...)>
{
  template <typename WorkletType,
            typename ThreadIndicesType,
            typename Device,
            typename ExecObjTuple>
  VTKM_NEVER_EXPORT static void Go(const WorkletType& worklet,
                                   const ThreadIndicesType& threadIndices,
                                   Device device,
                                   const ExecObjTuple& execObjectTuple)
  {
    // All input arguments are references starting at index 1. Also, index 0 is specially
    // reserved to hold the DeviceAdapterTag. Handle both of these by extending the control
    // tags and execObjects by one.
    using ExtendedControlTagList = detail::ExtendedControlSignatureToList<WorkletType>;

    // Get the fetch object associated with each executionTag (which is in turn associated with
    // each worklet argument) and load the associated worklet argument. Return a `Tuple` containing
    // `Pair`s of each `ExecTag` and its associated worklet argument.
    auto loadWorkletArg = [&](auto execTag) {
      using ExecTag = decltype(execTag);
      using ContTag = vtkm::ListAt<ExtendedControlTagList, ExecTag::INDEX>;
      using FetchTag = typename ContTag::FetchTag;
      auto&& execArg = detail::GetAdjustedExecObj(
        std::integral_constant<vtkm::IdComponent, ExecTag::INDEX>{}, execObjectTuple, device);
      vtkm::exec::arg::Fetch<FetchTag, typename ExecTag::AspectTag, std::decay_t<decltype(execArg)>>
        fetch;
      return vtkm::make_Pair(execTag, fetch.Load(threadIndices, execArg));
    };
    auto tagAndWorkletArgs = vtkm::MakeTuple(loadWorkletArg(ExecSigArgs{})...);
    //    tagAndWorkletArgs.foo();
    //    std::is_const<decltype(tagAndWorkletArgs)>::type::foo;

    // Call the worklet with the fetch arguments.
    auto callWorklet = [&](auto&... workletArgPairs) { worklet(workletArgPairs.second...); };
    tagAndWorkletArgs.Apply(callWorklet);

    // Store the result of the arguments.
    auto storeWorkletArg = [&](auto&& fetchAndArgPair) {
      using ExecTag = decltype(fetchAndArgPair.first);
      using ContTag = vtkm::ListAt<ExtendedControlTagList, ExecTag::INDEX>;
      using FetchTag = typename ContTag::FetchTag;
      auto&& execArg = detail::GetAdjustedExecObj(
        std::integral_constant<vtkm::IdComponent, ExecTag::INDEX>{}, execObjectTuple, device);
      vtkm::exec::arg::Fetch<FetchTag, typename ExecTag::AspectTag, std::decay_t<decltype(execArg)>>
        fetch;
      fetch.Store(threadIndices, execArg, fetchAndArgPair.second);
    };
    tagAndWorkletArgs.ForEach(storeWorkletArg);
  }
};

} // namespace detail

template <typename WorkletType, typename ThreadIndicesType, typename Device, typename ExecObjTuple>
VTKM_NEVER_EXPORT VTKM_EXEC void WorkletInvokeFunctor(const WorkletType& worklet,
                                                      const ThreadIndicesType& threadIndices,
                                                      Device device,
                                                      const ExecObjTuple& execObjectTuple)
{
  detail::DoWorkletInvokeFunctor<detail::ExecutionSignature<WorkletType>>::Go(
    worklet, threadIndices, device, execObjectTuple);
}

} // namespace vtkm::exec::internal
} // namespace vtkm::exec
} // namespace vtkm

#endif //vtk_m_exec_internal_WorkletInvokeFunctor_h
