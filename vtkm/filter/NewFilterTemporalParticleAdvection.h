//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_NewTemporalFilterParticleAdvection_h
#define vtk_m_filter_NewTemporalFilterParticleAdvection_h

#include <vtkm/filter/NewFilterParticleAdvection.h>
#include <vtkm/filter/vtkm_filter_core_export.h>

namespace vtkm
{
namespace filter
{
/// \brief base class for advecting particles in a vector field.

/// Takes as input a vector field and seed locations and advects the seeds
/// through the flow field.
class VTKM_FILTER_CORE_EXPORT NewFilterTemporalParticleAdvection
  : public vtkm::filter::NewFilterParticleAdvection
{
public:
  VTKM_CONT
  void SetPreviousTime(vtkm::FloatDefault t) { this->PreviousTime = t; }
  VTKM_CONT
  void SetNextTime(vtkm::FloatDefault t) { this->NextTime = t; }

  VTKM_CONT
  void SetNextDataSet(const vtkm::cont::DataSet& ds)
  {
    this->NextDataSet = vtkm::cont::PartitionedDataSet(ds);
  }

  VTKM_CONT
  void SetNextDataSet(const vtkm::cont::PartitionedDataSet& pds) { this->NextDataSet = pds; }

protected:
  VTKM_CONT void ValidateOptions(const vtkm::cont::PartitionedDataSet& input) const;
  using vtkm::filter::NewFilterParticleAdvection::ValidateOptions;

  using DSIType = vtkm::filter::particle_advection::TemporalDataSetIntegrator;
  VTKM_CONT std::vector<DSIType> CreateDataSetIntegrators(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::particle_advection::BoundsMap& boundsMap) const;

  vtkm::FloatDefault PreviousTime = 0;
  vtkm::FloatDefault NextTime = 0;
  vtkm::cont::PartitionedDataSet NextDataSet;
};

} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_NewTemporalFilterParticleAdvection_h
