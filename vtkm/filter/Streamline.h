//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Streamline_h
#define vtk_m_filter_Streamline_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/particle_advection //Streamline.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/particle_advection/Streamline.h instead of vtkm/filter/Streamline.h.")
inline void Streamline_deprecated() {}

inline void Streamline_deprecated_warning()
{
  Streamline_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::particle_advection::Streamline.") Streamline
  : public vtkm::filter::particle_advection::Streamline
{
  using particle_advection::Streamline::Streamline;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Streamline_h
