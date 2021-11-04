//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/vtkm_filter_common_export.h>

namespace vtkm
{
namespace filter
{
namespace internal
{
VTKM_FILTER_COMMON_EXPORT void RunFilter(Filter* self,
                                         vtkm::filter::DataSetQueue& input,
                                         vtkm::filter::DataSetQueue& output)
{
  auto filterClone = self->Clone();
  VTKM_ASSERT(filterClone != nullptr);

  std::pair<vtkm::Id, vtkm::cont::DataSet> task;
  while (input.GetTask(task))
  {
    auto outDS = CallPrepareForExecution(filterClone, task.second);
    //    filterClone->CallMapFieldOntoOutput(task.second, outDS, policy);
    output.Push(std::make_pair(task.first, std::move(outDS)));
  }

  vtkm::cont::Algorithm::Synchronize();
  delete filterClone;
}
} // internal

//----------------------------------------------------------------------------
Filter::Filter()
  : Invoke()
  , CoordinateSystemIndex(0)
  , FieldsToPass(vtkm::filter::FieldSelection::MODE_ALL)
{
}

//----------------------------------------------------------------------------
Filter::~Filter() = default;

//----------------------------------------------------------------------------
vtkm::cont::DataSet Filter::Execute(const vtkm::cont::DataSet& input)
{
  vtkm::cont::PartitionedDataSet output = this->Execute(vtkm::cont::PartitionedDataSet(input));
  if (output.GetNumberOfPartitions() > 1)
  {
    throw vtkm::cont::ErrorFilterExecution("Expecting at most 1 block.");
  }
  return output.GetNumberOfPartitions() == 1 ? output.GetPartition(0) : vtkm::cont::DataSet();
}

vtkm::cont::PartitionedDataSet Filter::Execute(const vtkm::cont::PartitionedDataSet& input)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                 "Filter (%d partitions): '%s'",
                 (int)input.GetNumberOfPartitions(),
                 vtkm::cont::TypeToString<decltype(*this)>().c_str());

  vtkm::filter::PolicyDefault policy;

  // Call `void Derived::PreExecute<DerivedPolicy>(input, policy)`, if defined.
  internal::CallPreExecute(this, input, policy);

  // Call `PrepareForExecution` (which should probably be renamed at some point)
  vtkm::cont::PartitionedDataSet output = internal::CallPrepareForExecution(this, input);

  // Call `Derived::PostExecute<DerivedPolicy>(input, output, policy)` if defined.
  internal::CallPostExecute(this, input, output, policy);
  return output;
}

vtkm::Id Filter::DetermineNumberOfThreads(const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::Id numDS = input.GetNumberOfPartitions();

  //Aribitrary constants.
  const vtkm::Id threadsPerGPU = 8;
  const vtkm::Id threadsPerCPU = 4;

  vtkm::Id availThreads = 1;

  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();

  if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagCuda{}))
    availThreads = threadsPerGPU;
  else if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagKokkos{}))
  {
    //Kokkos doesn't support threading on the CPU.
#ifdef VTKM_KOKKOS_CUDA
    availThreads = threadsPerGPU;
#else
    availThreads = 1;
#endif
  }
  else if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagSerial{}))
    availThreads = 1;
  else
    availThreads = threadsPerCPU;

  vtkm::Id numThreads = std::min<vtkm::Id>(numDS, availThreads);
  return numThreads;
}

// FIXME: unused, dup of Execute(PartitionedDataSet), to be removed
vtkm::cont::PartitionedDataSet Filter::ExecuteThreaded(const vtkm::cont::PartitionedDataSet& input,
                                                       vtkm::Id vtkmNotUsed(numThreads))
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                 "Filter (%d partitions): '%s'",
                 (int)input.GetNumberOfPartitions(),
                 vtkm::cont::TypeToString<decltype(*this)>().c_str());

  vtkm::filter::PolicyDefault policy;

  // Call `void Derived::PreExecute<DerivedPolicy>(input, policy)`, if defined.
  internal::CallPreExecute(this, input, policy);

  // Call `PrepareForExecution` (which should probably be renamed at some point)
  vtkm::cont::PartitionedDataSet output = internal::CallPrepareForExecution(this, input);

  // Call `Derived::PostExecute<DerivedPolicy>(input, output, policy)` if defined.
  internal::CallPostExecute(this, input, output, policy);
  return output;
}

//--------------------------------------------------------------------------------
void Filter::CallMapFieldOntoOutput(const vtkm::cont::DataSet& input, vtkm::cont::DataSet& output)
{
  for (vtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (this->GetFieldsToPass().IsFieldSelected(field))
    {
      this->MapFieldOntoOutput(output, field);
    }
  }
}

// FIXME: void return type?
bool Filter::MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field)
{
  // The default operation of mapping a field from input to output is to
  // just add the filed.
  result.AddField(field);
  return true;
}

} // namespace filter
} // namespace vtkm
