//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_internal_DispatcherBase_h
#define vtk_m_worklet_internal_DispatcherBase_h

#include <vtkm/StaticAssert.h>

#include <vtkm/internal/FunctionInterface.h>

#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/Transport.h>
#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>

#include <vtkm/internal/brigand.hpp>

#include <vtkm/internal/DecayHelpers.h>

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkmstd/is_trivial.h>

#include <sstream>

namespace vtkm
{
namespace cont
{

// Forward declaration.
template <typename CellSetList>
class DynamicCellSetBase;
}
}

namespace vtkm
{
namespace worklet
{
namespace internal
{

template <typename Domain>
inline auto SchedulingRange(const Domain& inputDomain) -> decltype(inputDomain.GetNumberOfValues())
{
  return inputDomain.GetNumberOfValues();
}

template <typename Domain>
inline auto SchedulingRange(const Domain* const inputDomain)
  -> decltype(inputDomain->GetNumberOfValues())
{
  return inputDomain->GetNumberOfValues();
}

template <typename Domain, typename SchedulingRangeType>
inline auto SchedulingRange(const Domain& inputDomain, SchedulingRangeType type)
  -> decltype(inputDomain.GetSchedulingRange(type))
{
  return inputDomain.GetSchedulingRange(type);
}

template <typename Domain, typename SchedulingRangeType>
inline auto SchedulingRange(const Domain* const inputDomain, SchedulingRangeType type)
  -> decltype(inputDomain->GetSchedulingRange(type))
{
  return inputDomain->GetSchedulingRange(type);
}

namespace detail
{

/// @{
/// \brief Returns the given parameter in a parameter pack.
///
template <typename T0, typename... Ts>
VTKM_NEVER_EXPORT auto GetArg(std::integral_constant<vtkm::IdComponent, 0>, T0&& a0, Ts&&...)
{
  return a0;
}
template <vtkm::IdComponent I, typename T0, typename... Ts>
VTKM_NEVER_EXPORT auto GetArg(std::integral_constant<vtkm::IdComponent, I>, T0&&, Ts&&... as)
{
  VTKM_STATIC_ASSERT(I > 0);
  return GetArg(std::integral_constant<vtkm::IdComponent, I - 1>{}, as...);
}
/// @}

// This code is actually taking an error found at compile-time and not
// reporting it until run-time. This seems strange at first, but this
// behavior is actually important. With dynamic arrays and similar dynamic
// classes, there may be types that are technically possible (such as using a
// vector where a scalar is expected) but in reality never happen. Thus, for
// these unsupported combinations we just silently halt the compiler from
// attempting to create code for these errant conditions and throw a run-time
// error if one every tries to create one.
inline void PrintFailureMessage(int index)
{
  std::stringstream message;
  message << "Encountered bad type for parameter " << index
          << " when calling Invoke on a dispatcher.";
  throw vtkm::cont::ErrorBadType(message.str());
}

template <typename T>
inline void not_nullptr(T* ptr, int index = -1, int mode = 0)
{
  if (!ptr)
  {
    std::stringstream message;
    message << "Encountered nullptr";
    if (index >= 0)
    {
      if (mode == 0)
      {
        message << " for parameter " << index;
      }
      else
      {
        message << " for " << index << " from last parameter ";
      }
    }
    else
    {
      message << " for parameter of type " << vtkm::cont::TypeToString(ptr);
    }
    message << " when calling Invoke on a dispatcher.";
    throw vtkm::cont::ErrorBadValue(message.str());
  }
}
template <typename T>
inline void not_nullptr(T&&, int index = -1, int mode = 0)
{
  (void)index;
  (void)mode;
}

template <typename T>
inline T& as_ref(T* ptr)
{
  return *ptr;
}
template <typename T>
inline T&& as_ref(T&& t)
{
  return std::forward<T>(t);
}


template <typename T, bool noError>
struct ReportTypeOnError;
template <typename T>
struct ReportTypeOnError<T, true> : std::true_type
{
};

template <int Value, bool noError>
struct ReportValueOnError;
template <int Value>
struct ReportValueOnError<Value, true> : std::true_type
{
};

// Is designed as a brigand fold operation.
template <typename Type, typename State>
struct DetermineIfHasDynamicParameter
{
  using T = vtkm::internal::remove_pointer_and_decay<Type>;
  using DynamicTag = typename vtkm::cont::internal::DynamicTransformTraits<T>::DynamicTag;
  using isDynamic =
    typename std::is_same<DynamicTag, vtkm::cont::internal::DynamicTransformTagCastAndCall>::type;

  using type = std::integral_constant<bool, (State::value || isDynamic::value)>;
};


// Is designed as a brigand fold operation.
template <typename WorkletType>
struct DetermineHasCorrectParameters
{
  template <typename Type, typename State, typename SigTypes>
  struct Functor
  {
    //T is the type of the Param at the current index
    //State if the index to use to fetch the control signature tag
    using ControlSignatureTag = typename brigand::at_c<SigTypes, State::value>;
    using TypeCheckTag = typename ControlSignatureTag::TypeCheckTag;

    using T = typename std::remove_pointer<Type>::type;
    static constexpr bool isCorrect = vtkm::cont::arg::TypeCheck<TypeCheckTag, T>::value;

    // If you get an error on the line below, that means that your code has called the
    // Invoke method on a dispatcher, and one of the arguments of the Invoke is the wrong
    // type. Each argument of Invoke corresponds to a tag in the arguments of the
    // ControlSignature of the worklet. If there is a mismatch, then you get an error here
    // (instead of where you called the dispatcher). For example, if the worklet has a
    // control signature as ControlSignature(CellSetIn, ...) and the first argument passed
    // to Invoke is an ArrayHandle, you will get an error here because you cannot use an
    // ArrayHandle in place of a CellSetIn argument. (You need to use a CellSet.) See a few
    // lines later for some diagnostics to help you trace where the error occurred.
    VTKM_READ_THE_SOURCE_CODE_FOR_HELP(isCorrect);

    // If you are getting the error described above, the following lines will give you some
    // diagnostics (in the form of compile errors). Each one will result in a compile error
    // reporting an undefined type for ReportTypeOnError (or ReportValueOnError). What we are
    // really reporting is the first template argument, which is one of the types or values that
    // should help pinpoint where the error is. The comment for static_assert provides the
    // type/value being reported. (Note that some compilers report better types than others. If
    // your compiler is giving unhelpful types like "T" or "WorkletType", you may need to try a
    // different compiler.)
    static_assert(ReportTypeOnError<T, isCorrect>::value, "Type passed to Invoke");
    static_assert(ReportTypeOnError<WorkletType, isCorrect>::value, "Worklet being invoked.");
    static_assert(ReportValueOnError<State::value, isCorrect>::value, "Index of Invoke parameter");
    static_assert(ReportTypeOnError<TypeCheckTag, isCorrect>::value, "Type check tag used");

    // This final static_assert gives a human-readable error message. Ideally, this would be
    // placed first, but some compilers will suppress further errors when a static_assert
    // fails, so you would not see the other diagnostic error messages.
    static_assert(isCorrect,
                  "The type of one of the arguments to the dispatcher's Invoke method is "
                  "incompatible with the corresponding tag in the worklet's ControlSignature.");

    using type = std::integral_constant<std::size_t, State::value + 1>;
  };
};

// Checks that an argument in a ControlSignature is a valid control signature
// tag. Causes a compile error otherwise.
struct DispatcherBaseControlSignatureTagCheck
{
  template <typename ControlSignatureTag, vtkm::IdComponent Index>
  struct ReturnType
  {
    // If you get a compile error here, it means there is something that is
    // not a valid control signature tag in a worklet's ControlSignature.
    VTKM_IS_CONTROL_SIGNATURE_TAG(ControlSignatureTag);
    using type = ControlSignatureTag;
  };
};

// Checks that an argument in a ExecutionSignature is a valid execution
// signature tag. Causes a compile error otherwise.
struct DispatcherBaseExecutionSignatureTagCheck
{
  template <typename ExecutionSignatureTag, vtkm::IdComponent Index>
  struct ReturnType
  {
    // If you get a compile error here, it means there is something that is not
    // a valid execution signature tag in a worklet's ExecutionSignature.
    VTKM_IS_EXECUTION_SIGNATURE_TAG(ExecutionSignatureTag);
    using type = ExecutionSignatureTag;
  };
};

VTKM_CONT
inline vtkm::Id FlatRange(vtkm::Id range)
{
  return range;
}

VTKM_CONT
inline vtkm::Id FlatRange(const vtkm::Id3& range)
{
  return range[0] * range[1] * range[2];
}

template <typename Device,
          typename InputDomainType,
          typename ControlSigTag,
          typename ControlObjectType>
VTKM_NEVER_EXPORT auto TransportObject(Device,
                                       vtkm::cont::Token& token,
                                       vtkm::Id inputSize,
                                       vtkm::Id outputSize,
                                       InputDomainType&& inputDomain,
                                       ControlSigTag,
                                       ControlObjectType&& controlObject)
{
  using TransportTag = typename ControlSigTag::TransportTag;
  vtkm::cont::arg::
    Transport<TransportTag, vtkm::internal::remove_pointer_and_decay<ControlObjectType>, Device>
      transport;

  not_nullptr(controlObject);
  auto execObject = transport(as_ref(controlObject),
                              as_ref(inputDomain),
                              inputSize,
                              outputSize,
                              token);

  // If you get a compile error here, it means that an execution object type is not
  // trivially copyable. This is strictly disallowed. All execution objects must be
  // trivially copyable so that they can be memcpy-ed between host and devices.
  // Note that it is still legal for execution objects to have pointers or other
  // references to resources on a particular device. It is up to the generating code
  // to ensure that all referenced resources are valid on the target device.
  VTKM_IS_TRIVIALLY_COPYABLE(decltype(execObject));

  return execObject;
}

// Some template tricks to transport objects from the control environment to
// the execution environment based on the associated transport tag in the
// `ControlSignature`.
template <typename ControlSignature>
struct VTKM_NEVER_EXPORT TransportHelper;

template <typename... ContSigTags>
struct VTKM_NEVER_EXPORT TransportHelper<void(ContSigTags...)>
{
  template <typename Functor,
            typename Device,
            typename InputRangeType,
            typename OutputRangeType,
            typename InputDomainType,
            typename... ControlObjectTypes>
  static auto Go(Functor&& functor,
                 Device device,
                 vtkm::cont::Token& token,
                 const InputRangeType& inputRange,
                 const OutputRangeType& outputRange,
                 InputDomainType&& inputDomain,
                 ControlObjectTypes&&... controlObjects)
  {
    vtkm::Id inputSize = FlatRange(inputRange);
    vtkm::Id outputSize = FlatRange(outputRange);

    return functor(TransportObject(device,
                                   token,
                                   inputSize,
                                   outputSize,
                                   std::forward<InputDomainType>(inputDomain),
                                   ContSigTags{},
                                   std::forward<ControlObjectTypes>(controlObjects))...);
  }
};

//forward declares
template <std::size_t LeftToProcess>
struct for_each_dynamic_arg;

template <std::size_t LeftToProcess, typename TypeCheckTag>
struct convert_arg_wrapper
{
  template <typename T, typename... Args>
  void operator()(T&& t, Args&&... args) const
  {
    using Type = typename std::decay<T>::type;
    using valid =
      std::integral_constant<bool, vtkm::cont::arg::TypeCheck<TypeCheckTag, Type>::value>;
    this->WillContinue(valid(), std::forward<T>(t), std::forward<Args>(args)...);
  }
  template <typename T, typename... Args>
  void WillContinue(std::true_type, T&& t, Args&&... args) const
  {
    for_each_dynamic_arg<LeftToProcess - 1>()(std::forward<Args>(args)..., std::forward<T>(t));
  }
  template <typename... Args>
  void WillContinue(std::false_type, Args&&...) const
  {
    vtkm::worklet::internal::detail::PrintFailureMessage(LeftToProcess);
  }
};

template <std::size_t LeftToProcess,
          typename T,
          typename ContParams,
          typename Trampoline,
          typename... Args>
inline void convert_arg(vtkm::cont::internal::DynamicTransformTagStatic,
                        T&& t,
                        const ContParams&,
                        const Trampoline& trampoline,
                        Args&&... args)
{ //This is a static array, so just push it to the back
  using popped_sig = brigand::pop_front<ContParams>;
  for_each_dynamic_arg<LeftToProcess - 1>()(
    trampoline, popped_sig(), std::forward<Args>(args)..., std::forward<T>(t));
}

template <std::size_t LeftToProcess,
          typename T,
          typename ContParams,
          typename Trampoline,
          typename... Args>
inline void convert_arg(vtkm::cont::internal::DynamicTransformTagCastAndCall,
                        T&& t,
                        const ContParams&,
                        const Trampoline& trampoline,
                        Args&&... args)
{ //This is something dynamic so cast and call
  using tag_check = typename brigand::at_c<ContParams, 0>::TypeCheckTag;
  using popped_sig = brigand::pop_front<ContParams>;

  not_nullptr(t, LeftToProcess, 1);
  vtkm::cont::CastAndCall(as_ref(t),
                          convert_arg_wrapper<LeftToProcess, tag_check>(),
                          trampoline,
                          popped_sig(),
                          std::forward<Args>(args)...);
}

template <std::size_t LeftToProcess>
struct for_each_dynamic_arg
{
  template <typename Trampoline, typename ContParams, typename T, typename... Args>
  void operator()(const Trampoline& trampoline, ContParams&& sig, T&& t, Args&&... args) const
  {
    //Determine that state of T when it is either a `cons&` or a `* const&`
    using Type = vtkm::internal::remove_pointer_and_decay<T>;
    using tag = typename vtkm::cont::internal::DynamicTransformTraits<Type>::DynamicTag;
    //convert the first item to a known type
    convert_arg<LeftToProcess>(
      tag(), std::forward<T>(t), sig, trampoline, std::forward<Args>(args)...);
  }
};

template <>
struct for_each_dynamic_arg<0>
{
  template <typename Trampoline, typename ContParams, typename... Args>
  void operator()(const Trampoline& trampoline, ContParams&&, Args&&... args) const
  {
    trampoline.StartInvokeDynamic(std::false_type(), std::forward<Args>(args)...);
  }
};

template <typename Trampoline, typename ContParams, typename... Args>
inline void deduce(Trampoline&& trampoline, ContParams&& sig, Args&&... args)
{
  for_each_dynamic_arg<sizeof...(Args)>()(std::forward<Trampoline>(trampoline), sig, args...);
}

} // namespace detail

/// This is a help struct to detect out of bound placeholders defined in the
/// execution signature at compile time
template <vtkm::IdComponent MaxIndexAllowed>
struct PlaceholderValidator
{
  PlaceholderValidator() {}

  // An overload operator to detect possible out of bound placeholder
  template <int N>
  void operator()(brigand::type_<vtkm::placeholders::Arg<N>>) const
  {
    static_assert(N <= MaxIndexAllowed,
                  "An argument in the execution signature"
                  " (usually _2, _3, _4, etc.) refers to a control signature argument that"
                  " does not exist. For example, you will get this error if you have _3 (or"
                  " _4 or _5 or so on) as one of the execution signature arguments, but you"
                  " have fewer than 3 (or 4 or 5 or so on) arguments in the control signature.");
  }

  template <typename DerivedType>
  void operator()(brigand::type_<DerivedType>) const
  {
  }
};

/// Base class for all dispatcher classes. Every worklet type should have its
/// own dispatcher.
///
template <typename DerivedClass, typename WorkletType, typename BaseWorkletType>
class DispatcherBase
{
private:
  using MyType = DispatcherBase<DerivedClass, WorkletType, BaseWorkletType>;

  friend struct detail::for_each_dynamic_arg<0>;

protected:
  using ControlInterface =
    vtkm::internal::FunctionInterface<typename WorkletType::ControlSignature>;

  // We go through the GetExecSig as that generates a default ExecutionSignature
  // if one doesn't exist on the worklet
  using ExecutionSignature =
    typename vtkm::placeholders::GetExecSig<WorkletType>::ExecutionSignature;
  using ExecutionInterface = vtkm::internal::FunctionInterface<ExecutionSignature>;

  static constexpr vtkm::IdComponent NUM_INVOKE_PARAMS = ControlInterface::ARITY;

private:
  // We don't really need these types, but declaring them checks the arguments
  // of the control and execution signatures.
  using ControlSignatureCheck = typename ControlInterface::template StaticTransformType<
    detail::DispatcherBaseControlSignatureTagCheck>::type;
  using ExecutionSignatureCheck = typename ExecutionInterface::template StaticTransformType<
    detail::DispatcherBaseExecutionSignatureTagCheck>::type;

  template <typename... Args>
  VTKM_CONT void StartInvoke(Args&&... args) const
  {
    using ParameterInterface =
      vtkm::internal::FunctionInterface<void(vtkm::internal::remove_cvref<Args>...)>;

    VTKM_STATIC_ASSERT_MSG(ParameterInterface::ARITY == NUM_INVOKE_PARAMS,
                           "Dispatcher Invoke called with wrong number of arguments.");

    static_assert(
      std::is_base_of<BaseWorkletType, WorkletType>::value,
      "The worklet being scheduled by this dispatcher doesn't match the type of the dispatcher");

    // Check if the placeholders defined in the execution environment exceed the max bound
    // defined in the control environment by throwing a nice compile error.
    using ComponentSig = typename ExecutionInterface::ComponentSig;
    brigand::for_each<ComponentSig>(PlaceholderValidator<NUM_INVOKE_PARAMS>{});

    //We need to determine if we have the need to do any dynamic
    //transforms. This is fairly simple of a query. We just need to check
    //everything in the FunctionInterface and see if any of them have the
    //proper dynamic trait. Doing this, allows us to generate zero dynamic
    //check & convert code when we already know all the types. This results
    //in smaller executables and libraries.
    using ParamTypes = typename ParameterInterface::ParameterSig;
    using HasDynamicTypes =
      brigand::fold<ParamTypes,
                    std::false_type,
                    detail::DetermineIfHasDynamicParameter<brigand::_element, brigand::_state>>;

    this->StartInvokeDynamic(HasDynamicTypes(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  VTKM_CONT void StartInvokeDynamic(std::true_type, Args&&... args) const
  {
    // As we do the dynamic transform, we are also going to check the static
    // type against the TypeCheckTag in the ControlSignature tags. To do this,
    // the check needs access to both the parameter (in the parameters
    // argument) and the ControlSignature tags (in the ControlInterface type).
    using ContParamsInfo =
      vtkm::internal::detail::FunctionSigInfo<typename WorkletType::ControlSignature>;
    typename ContParamsInfo::Parameters parameters;
    detail::deduce(*this, parameters, std::forward<Args>(args)...);
  }

  template <typename... Args>
  VTKM_CONT void StartInvokeDynamic(std::false_type, Args&&... args) const
  {
    using ParameterInterface =
      vtkm::internal::FunctionInterface<void(vtkm::internal::remove_cvref<Args>...)>;

    //Nothing requires a conversion from dynamic to static types, so
    //next we need to verify that each argument's type is correct. If not
    //we need to throw a nice compile time error
    using ParamTypes = typename ParameterInterface::ParameterSig;
    using ContSigTypes = typename vtkm::internal::detail::FunctionSigInfo<
      typename WorkletType::ControlSignature>::Parameters;

    //isAllValid will throw a compile error if everything doesn't match
    using isAllValid = brigand::fold<
      ParamTypes,
      std::integral_constant<std::size_t, 0>,
      typename detail::DetermineHasCorrectParameters<WorkletType>::
        template Functor<brigand::_element, brigand::_state, brigand::pin<ContSigTypes>>>;

    //this warning exists so that we don't get a warning from not using isAllValid
    using expectedLen = std::integral_constant<std::size_t, sizeof...(Args)>;
    static_assert(isAllValid::value == expectedLen::value,
                  "All arguments failed the TypeCheck pass");

    static_cast<const DerivedClass*>(this)->DoInvoke(std::forward<Args>(args)...);
  }

public:
  //@{
  /// Setting the device ID will force the execute to happen on a particular device. If no device
  /// is specified (or the device ID is set to any), then a device will automatically be chosen
  /// based on the runtime device tracker.
  ///
  VTKM_CONT
  void SetDevice(vtkm::cont::DeviceAdapterId device) { this->Device = device; }

  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const { return this->Device; }
  //@}

  using ScatterType = typename WorkletType::ScatterType;
  using MaskType = typename WorkletType::MaskType;

  template <typename... Args>
  VTKM_CONT void Invoke(Args&&... args) const
  {
    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                   "Invoking Worklet: '%s'",
                   vtkm::cont::TypeToString<DerivedClass>().c_str());
    this->StartInvoke(std::forward<Args>(args)...);
  }

protected:
  // If you get a compile error here about there being no appropriate constructor for ScatterType
  // or MapType, then that probably means that the worklet you are trying to execute has defined a
  // custom ScatterType or MaskType and that you need to create one (because there is no default
  // way to construct the scatter or mask).
  VTKM_CONT
  DispatcherBase(const WorkletType& worklet = WorkletType(),
                 const ScatterType& scatter = ScatterType(),
                 const MaskType& mask = MaskType())
    : Worklet(worklet)
    , Scatter(scatter)
    , Mask(mask)
    , Device(vtkm::cont::DeviceAdapterTagAny())
  {
  }

  // If you get a compile error here about there being no appropriate constructor for MaskType,
  // then that probably means that the worklet you are trying to execute has defined a custom
  // MaskType and that you need to create one (because there is no default way to construct the
  // mask).
  VTKM_CONT
  DispatcherBase(const ScatterType& scatter, const MaskType& mask = MaskType())
    : Worklet(WorkletType())
    , Scatter(scatter)
    , Mask(mask)
    , Device(vtkm::cont::DeviceAdapterTagAny())
  {
  }

  // If you get a compile error here about there being no appropriate constructor for ScatterType,
  // then that probably means that the worklet you are trying to execute has defined a custom
  // ScatterType and that you need to create one (because there is no default way to construct the
  // scatter).
  VTKM_CONT
  DispatcherBase(const WorkletType& worklet,
                 const MaskType& mask,
                 const ScatterType& scatter = ScatterType())
    : Worklet(worklet)
    , Scatter(scatter)
    , Mask(mask)
    , Device(vtkm::cont::DeviceAdapterTagAny())
  {
  }

  // If you get a compile error here about there being no appropriate constructor for ScatterType,
  // then that probably means that the worklet you are trying to execute has defined a custom
  // ScatterType and that you need to create one (because there is no default way to construct the
  // scatter).
  VTKM_CONT
  DispatcherBase(const MaskType& mask, const ScatterType& scatter = ScatterType())
    : Worklet(WorkletType())
    , Scatter(scatter)
    , Mask(mask)
    , Device(vtkm::cont::DeviceAdapterTagAny())
  {
  }

  /// Given the parameters provided to an Invoke, returns the parameter associated with the input
  /// domain.
  template <typename... Args>
  VTKM_NEVER_EXPORT auto GetInputDomain(Args&&... args) const
  {
    // Note: InputDomain is indexed starting at 1 instead of 0.
    return detail::GetArg(
      std::integral_constant<vtkm::IdComponent, WorkletType::InputDomain::INDEX - 1>{}, args...);
  }

  template <typename InputRangeType, typename... ControlObjectTypes>
  VTKM_NEVER_EXPORT void BasicInvoke(InputRangeType inputRange,
                                     ControlObjectTypes&&... controlObjects) const
  {
    bool success = vtkm::cont::TryExecuteOnDevice(this->Device, [&](auto device) {
      this->InvokeTransportSchedule(
        inputRange, device, std::forward<ControlObjectTypes>(controlObjects)...);
      return true;
    });
    if (!success)
    {
      throw vtkm::cont::ErrorExecution("Failed to execute worklet on any device.");
    }
  }

  template <typename... ControlObjectTypes>
  VTKM_NEVER_EXPORT void BasicInvoke(vtkm::Id2 inputRange,
                                     ControlObjectTypes&&... controlObjects) const
  {
    this->BasicInvoke(vtkm::Id3{ inputRange[0], inputRange[1], 1 },
                      std::forward<ControlObjectTypes>(controlObjects)...);
  }

  WorkletType Worklet;
  ScatterType Scatter;
  MaskType Mask;

private:
  // Dispatchers cannot be copied
  DispatcherBase(const MyType&) = delete;
  void operator=(const MyType&) = delete;

  vtkm::cont::DeviceAdapterId Device;

  template <typename InputRangeType, typename Device, typename... ControlObjectTypes>
  VTKM_NEVER_EXPORT void InvokeTransportSchedule(const InputRangeType& inputRange,
                                                 Device device,
                                                 ControlObjectTypes&&... controlObjects) const
  {
    // This token represents the scope of the execution objects. It should
    // exist as long as things run on the device.
    vtkm::cont::Token token;

    // Get the arrays used for scattering input to output.
    auto outputToInputMap = this->Scatter.GetOutputToInputMap(inputRange);
    auto visitArray = this->Scatter.GetVisitArray(inputRange);
    auto outputRange = this->Scatter.GetOutputRange(inputRange);

    // Get the arrays used for masking output elements.
    auto threadToOutputMap = this->Mask.GetThreadToOutputMap(inputRange);
    auto threadRange = this->Mask.GetThreadRange(outputRange);

    // Before the worklet is scheduled, the invoke parameters, which are defined in
    // the control environment, have to be transported to the execution environment
    // associated with the given device.
    auto task = detail::TransportHelper<typename WorkletType::ControlSignature>::Go(
      [&](auto&&... execObjects) {
        return vtkm::cont::DeviceTaskTypes<Device>::MakeTask(
          this->Worklet,
          outputToInputMap.PrepareForInput(device, token),
          visitArray.PrepareForInput(device, token),
          threadToOutputMap.PrepareForInput(device, token),
          threadRange,
          execObjects...);
      },
      device,
      token,
      inputRange,
      outputRange,
      this->GetInputDomain(controlObjects...),
      std::forward<ControlObjectTypes>(controlObjects)...);

    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScheduleTask(task, threadRange);
  }
};
}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_DispatcherBase_h
