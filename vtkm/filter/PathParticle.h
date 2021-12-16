//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_PathParticle_h
#define vtk_m_filter_PathParticle_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/particle_advection //PathParticle.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/particle_advection/PathParticle.h instead of vtkm/filter/PathParticle.h.")
inline void PathParticle_deprecated() {}

inline void PathParticle_deprecated_warning()
{
  PathParticle_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::particle_advection::PathParticle.") PathParticle
  : public vtkm::filter::particle_advection::PathParticle
{
  using particle_advection::PathParticle::PathParticle;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_PathParticle_h
