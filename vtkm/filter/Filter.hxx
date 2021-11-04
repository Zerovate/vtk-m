//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/PolicyDefault.h>

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>

#include <vtkm/filter/TaskQueue.h>

#include <future>

namespace vtkm
{
namespace filter
{

namespace internal
{
extern void RunFilter(Filter* self,
                      vtkm::filter::DataSetQueue& input,
                      vtkm::filter::DataSetQueue& output);

template <typename T, typename InputType, typename DerivedPolicy>
struct SupportsPreExecute
{
  template <typename U,
            typename S = decltype(std::declval<U>().PreExecute(
              std::declval<InputType>(),
              std::declval<vtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};

template <typename T, typename InputType, typename DerivedPolicy>
struct SupportsPostExecute
{
  template <typename U,
            typename S = decltype(std::declval<U>().PostExecute(
              std::declval<InputType>(),
              std::declval<InputType&>(),
              std::declval<vtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};


template <typename T, typename InputType>
struct SupportsPrepareForExecution
{
  template <typename U,
            typename S = decltype(std::declval<U>().PrepareForExecution(std::declval<InputType>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPreExecuteInternal(std::true_type, Derived* self, Args&&... args)
{
  return self->PreExecute(std::forward<Args>(args)...);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPreExecuteInternal(std::false_type, Derived*, Args&&...)
{
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPreExecute(Derived* self,
                    const InputType& input,
                    const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  using call_supported_t = typename SupportsPreExecute<Derived, InputType, DerivedPolicy>::type;
  CallPreExecuteInternal(call_supported_t(), self, input, policy);
}

//--------------------------------------------------------------------------------
// forward declare.
template <typename Derived, typename InputType>
InputType CallPrepareForExecution(Derived* self, const InputType& input);

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType>
InputType CallPrepareForExecutionInternal(std::true_type, Derived* self, const InputType& input)
{
  return self->PrepareForExecution(input);
}


//--------------------------------------------------------------------------------
// specialization for PartitionedDataSet input when `PrepareForExecution` is not provided
// by the subclass. we iterate over blocks and execute for each block
// individually.
template <typename Derived>
vtkm::cont::PartitionedDataSet CallPrepareForExecutionInternal(
  std::false_type,
  Derived* self,
  const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::cont::PartitionedDataSet output;

  if (self->GetRunMultiThreadedFilter())
  {
    vtkm::filter::DataSetQueue inputQueue(input);
    vtkm::filter::DataSetQueue outputQueue;

    vtkm::Id numThreads = self->DetermineNumberOfThreads(input);

    //Run 'numThreads' filters.
    std::vector<std::future<void>> futures(static_cast<std::size_t>(numThreads));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numThreads); i++)
    {
      auto f = std::async(
        std::launch::async, RunFilter, self, std::ref(inputQueue), std::ref(outputQueue));
      futures[i] = std::move(f);
    }

    for (auto& f : futures)
      f.get();

    //Get results from the outputQueue.
    output = outputQueue.Get();
  }
  else
  {
    for (const auto& inBlock : input)
    {
      vtkm::cont::DataSet outBlock = CallPrepareForExecution(self, inBlock);
      output.AppendPartition(outBlock);
    }
  }

  return output;
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType>
InputType CallPrepareForExecution(Derived* self, const InputType& input)
{
  using call_supported_t = typename SupportsPrepareForExecution<Derived, InputType>::type;
  return CallPrepareForExecutionInternal(call_supported_t(), self, input);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPostExecuteInternal(std::true_type,
                             Derived* self,
                             const InputType& input,
                             InputType& output,
                             const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  self->PostExecute(input, output, policy);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPostExecuteInternal(std::false_type, Derived*, Args&&...)
{
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPostExecute(Derived* self,
                     const InputType& input,
                     InputType& output,
                     const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  using call_supported_t = typename SupportsPostExecute<Derived, InputType, DerivedPolicy>::type;
  CallPostExecuteInternal(call_supported_t(), self, input, output, policy);
}
}


//----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DataSet Filter::Execute(const vtkm::cont::DataSet& input,
                                              vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //  Derived* self = static_cast<Derived*>(this);
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  vtkm::cont::PartitionedDataSet output =
    this->Execute(vtkm::cont::PartitionedDataSet(input), policy);
  VTKM_DEPRECATED_SUPPRESS_END
  if (output.GetNumberOfPartitions() > 1)
  {
    throw vtkm::cont::ErrorFilterExecution("Expecting at most 1 block.");
  }
  return output.GetNumberOfPartitions() == 1 ? output.GetPartition(0) : vtkm::cont::DataSet();
}

//----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::PartitionedDataSet Filter::Execute(
  const vtkm::cont::PartitionedDataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                 "Filter (%d partitions): '%s'",
                 (int)input.GetNumberOfPartitions(),
                 vtkm::cont::TypeToString<decltype((*this))>().c_str());

  //  Derived* self = static_cast<Derived*>(this);

  // Call `void Derived::PreExecute<DerivedPolicy>(input, policy)`, if defined.
  internal::CallPreExecute(this, input, policy);

  // Call `PrepareForExecution` (which should probably be renamed at some point)
  vtkm::cont::PartitionedDataSet output = internal::CallPrepareForExecution(this, input, policy);

  // Call `Derived::PostExecute<DerivedPolicy>(input, output, policy)` if defined.
  internal::CallPostExecute(this, input, output, policy);
  return output;
}

}
}
