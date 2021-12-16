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
#include <vtkm/filter/NewFilterParticleAdvection.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
void NewFilterParticleAdvection::ValidateOptions() const
{
  if (this->GetUseCoordinateSystemAsField())
    throw vtkm::cont::ErrorFilterExecution("Coordinate system as field not supported");
  if (this->Seeds.GetNumberOfValues() == 0)
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");
  if (this->NumberOfSteps == 0)
    throw vtkm::cont::ErrorFilterExecution("Number of steps not specified.");
  if (this->StepSize == 0)
    throw vtkm::cont::ErrorFilterExecution("Step size not specified.");
}

std::vector<vtkm::filter::particle_advection::DataSetIntegrator>
NewFilterParticleAdvection::CreateDataSetIntegrators(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::particle_advection::BoundsMap& boundsMap) const
{
  std::vector<vtkm::filter::particle_advection::DataSetIntegrator> dsi;

  if (boundsMap.GetTotalNumBlocks() == 0)
    throw vtkm::cont::ErrorFilterExecution("No input datasets.");

  std::string activeField = this->GetActiveFieldName();

  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds = input.GetPartition(i);
    if (!ds.HasPointField(activeField))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field association");
    dsi.emplace_back(ds, blockId, activeField);
  }

  return dsi;
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet NewFilterParticleAdvection::DoExecute(
  const vtkm::cont::DataSet& input)
{
  vtkm::cont::PartitionedDataSet output = this->Execute(vtkm::cont::PartitionedDataSet(input));
  return output.GetPartition(0);
}

}
} // namespace vtkm::filter
