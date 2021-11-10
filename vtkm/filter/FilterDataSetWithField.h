//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DataSetWithFieldFilter_h
#define vtk_m_filter_DataSetWithFieldFilter_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

class FilterDataSetWithField
  : public vtkm::filter::FilterField
  , public vtkm::filter::FilterDataSet
{
public:
  VTKM_CONT
  FilterDataSetWithField() = default;

  VTKM_CONT
  ~FilterDataSetWithField() = default;

protected:
  vtkm::filter::FilterDataSetWithField& operator=(const vtkm::filter::FilterDataSetWithField&) =
    default;

private:
  friend class vtkm::filter::Filter;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_DataSetWithFieldFilter_h
