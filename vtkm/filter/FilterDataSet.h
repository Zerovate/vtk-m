//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DataSetFilter_h
#define vtk_m_filter_DataSetFilter_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

class FilterDataSet : public virtual vtkm::filter::Filter
{
public:
  VTKM_CONT
  FilterDataSet() = default;

  VTKM_CONT
  ~FilterDataSet() = default;

protected:
  vtkm::filter::FilterDataSet& operator=(const vtkm::filter::FilterDataSet&) = default;
  VTKM_CONT
  void CopyStateFrom(const FilterDataSet* filter) { *this = *filter; }

private:
  friend class vtkm::filter::Filter;
};
}
} // namespace vtkm::filter

//#include <vtkm/filter/FilterDataSet.hxx>

#endif // vtk_m_filter_DataSetFilter_h
