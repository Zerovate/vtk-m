//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Tetrahedralize_h
#define vtk_m_filter_Tetrahedralize_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/GeometryGeneration/vtkm_filter_geometrygeneration_export.h>

namespace vtkm
{
namespace filter
{

class VTKM_FILTER_GEOMETRYGENERATION_EXPORT Tetrahedralize : public vtkm::filter::Filter
{
public:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_Tetrahedralize_h
