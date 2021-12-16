//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/particle_advection/BoundsMap.h>
#include <vtkm/filter/particle_advection/ParticleAdvectionAlgorithm.h>
#include <vtkm/filter/particle_advection/Streamline.h>

namespace vtkm
{
namespace filter
{
namespace particle_advection
{
VTKM_CONT vtkm::cont::PartitionedDataSet Streamline::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  using AlgorithmType = vtkm::filter::particle_advection::StreamlineAlgorithm;
  using ThreadedAlgorithmType = vtkm::filter::particle_advection::StreamlineThreadedAlgorithm;

  this->ValidateOptions();
  vtkm::filter::particle_advection::BoundsMap boundsMap(input);
  auto dsi = this->CreateDataSetIntegrators(input, boundsMap);

  if (this->GetUseThreadedAlgorithm())
    return vtkm::filter::particle_advection::RunAlgo<DSIType, ThreadedAlgorithmType>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
  else
    return vtkm::filter::particle_advection::RunAlgo<DSIType, AlgorithmType>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
}

} // namespace particle_advection
} // namespace filter
} // namespace vtkm
