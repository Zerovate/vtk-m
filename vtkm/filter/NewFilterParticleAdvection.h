//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_NewFilterParticleAdvection_h
#define vtk_m_filter_NewFilterParticleAdvection_h

#include <vtkm/Particle.h>
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/particle_advection/BoundsMap.h>
#include <vtkm/filter/particle_advection/DataSetIntegrator.h> // FIXME: includes worklet/* ?
#include <vtkm/filter/vtkm_filter_core_export.h>

namespace vtkm
{
namespace filter
{
/// \brief base class for advecting particles in a vector field.

/// Takes as input a vector field and seed locations and advects the seeds
/// through the flow field.
class VTKM_FILTER_CORE_EXPORT NewFilterParticleAdvection : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT
  void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }

  VTKM_CONT
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }

  VTKM_CONT
  void SetSeeds(const std::vector<vtkm::Particle>& seeds,
                vtkm::CopyFlag copyFlag = vtkm::CopyFlag::On)
  {
    this->Seeds = vtkm::cont::make_ArrayHandle(seeds, copyFlag);
  }

  VTKM_CONT
  void SetSeeds(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds) { this->Seeds = seeds; }

  VTKM_CONT
  bool GetUseThreadedAlgorithm() { return this->UseThreadedAlgorithm; }

  VTKM_CONT
  void SetUseThreadedAlgorithm(bool val) { this->UseThreadedAlgorithm = val; }

protected:
  VTKM_CONT virtual void ValidateOptions() const;

  using DSIType = vtkm::filter::particle_advection::DataSetIntegrator;
  VTKM_CONT std::vector<DSIType> CreateDataSetIntegrators(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::particle_advection::BoundsMap& boundsMap) const;

  vtkm::Id NumberOfSteps = 0;
  vtkm::FloatDefault StepSize = 0;
  vtkm::cont::ArrayHandle<vtkm::Particle> Seeds;
  bool UseThreadedAlgorithm = false;

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_NewFilterParticleAdvection_h
