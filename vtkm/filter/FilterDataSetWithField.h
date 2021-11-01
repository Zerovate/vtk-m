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

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

template <class Derived>
class FilterDataSetWithField
  : public vtkm::filter::FilterField<Derived>
  , public vtkm::filter::FilterDataSet<Derived>
{
public:
  VTKM_CONT
  FilterDataSetWithField() = default;

  VTKM_CONT
  ~FilterDataSetWithField() = default;

protected:
  vtkm::filter::FilterDataSetWithField<Derived>& operator=(
    const vtkm::filter::FilterDataSetWithField<Derived>&) = default;

  VTKM_CONT
  void CopyStateFrom(const FilterDataSetWithField<Derived>* filter) { *this = *filter; }

private:
  friend class vtkm::filter::Filter<Derived>;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_DataSetWithFieldFilter_h
