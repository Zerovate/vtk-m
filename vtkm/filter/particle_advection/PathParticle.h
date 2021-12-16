//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particle_advection_PathParticle_h
#define vtk_m_filter_particle_advection_PathParticle_h

#include <vtkm/filter/NewFilterTemporalParticleAdvection.h>
#include <vtkm/filter/particle_advection/vtkm_filter_particle_advection_export.h>

namespace vtkm
{
namespace filter
{
namespace particle_advection
{
/// \brief Advect particles in a time varying vector field.

/// Takes as input a vector field and seed locations and generates the
/// paths taken by the seeds through the vector field.
class VTKM_FILTER_PARTICLE_ADVECTION_EXPORT PathParticle
  : public vtkm::filter::NewFilterTemporalParticleAdvection
{
private:
  vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& input) override;
};

}
}
} // namespace vtkm::filter

#endif // vtk_m_filter_particle_advection_PathParticle_h
