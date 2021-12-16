//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Pathline_h
#define vtk_m_filter_Pathline_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/particle_advection //Pathline.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/particle_advection/Pathline.h instead of vtkm/filter/Pathline.h.")
inline void Pathline_deprecated() {}

inline void Pathline_deprecated_warning()
{
  Pathline_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::particle_advection::Pathline.") Pathline
  : public vtkm::filter::particle_advection::Pathline
{
  using particle_advection::Pathline::Pathline;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Pathline_h
