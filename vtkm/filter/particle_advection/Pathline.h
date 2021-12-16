//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particle_advection_Pathline_h
#define vtk_m_filter_particle_advection_Pathline_h

#include <vtkm/filter/NewFilterTemporalParticleAdvection.h>
#include <vtkm/filter/particle_advection/vtkm_filter_particle_advection_export.h>

namespace vtkm
{
namespace filter
{
namespace particle_advection
{
/// \brief generate pathlines from a time sequence of vector fields.

/// Takes as input a vector field and seed locations and generates the
/// paths taken by the seeds through the vector field.
class VTKM_FILTER_PARTICLE_ADVECTION_EXPORT Pathline
  : public vtkm::filter::NewFilterTemporalParticleAdvection
{
private:
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& input) override;
};

} // namespace particle_advection
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_Pathline_h
