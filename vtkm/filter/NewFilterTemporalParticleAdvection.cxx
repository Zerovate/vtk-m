//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/NewFilterTemporalParticleAdvection.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
void NewFilterTemporalParticleAdvection::ValidateOptions(
  const vtkm::cont::PartitionedDataSet& input) const
{
  this->vtkm::filter::NewFilterParticleAdvection::ValidateOptions();

  if (this->NextDataSet.GetNumberOfPartitions() != input.GetNumberOfPartitions())
    throw vtkm::cont::ErrorFilterExecution("Number of partitions do not match");
  if (this->PreviousTime >= this->NextTime)
    throw vtkm::cont::ErrorFilterExecution("Previous time must be less than Next time.");
}

std::vector<vtkm::filter::particle_advection::TemporalDataSetIntegrator>
NewFilterTemporalParticleAdvection::CreateDataSetIntegrators(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::particle_advection::BoundsMap& boundsMap) const
{
  std::vector<DSIType> dsi;
  std::string activeField = this->GetActiveFieldName();

  if (boundsMap.GetTotalNumBlocks() == 0)
    throw vtkm::cont::ErrorFilterExecution("No input datasets.");

  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto dsPrev = input.GetPartition(i);
    auto dsNext = this->NextDataSet.GetPartition(i);
    if (!dsPrev.HasPointField(activeField) || !dsNext.HasPointField(activeField))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
    dsi.emplace_back(dsPrev, this->PreviousTime, dsNext, this->NextTime, blockId, activeField);
  }

  return dsi;
}

} // namespace filter
} // namespace vtkm
