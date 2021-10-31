//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Slice_cxx
#define vtk_m_filter_Slice_cxx

#include <vtkm/filter/Contour/Contour.h>
#include <vtkm/filter/Contour/Contour.hxx>
#include <vtkm/filter/Contour/Slice.h>
#include <vtkm/filter/Contour/Slice.hxx>

namespace vtkm
{
namespace filter
{

template VTKM_FILTER_CONTOUR_EXPORT vtkm::cont::DataSet Slice::DoExecute(
  const vtkm::cont::DataSet&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);

}
} // vtkm::filter

#endif // vtk_m_filter_Slice_cxx
