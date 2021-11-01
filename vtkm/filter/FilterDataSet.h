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

template <class Derived>
class FilterDataSet : public virtual vtkm::filter::Filter<Derived>
{
public:
  VTKM_CONT
  FilterDataSet();

  VTKM_CONT
  ~FilterDataSet();

  /// These are provided to satisfy the Filter API requirements.

  //From the field we can extract the association component
  // Association::ANY -> unable to map
  // Association::WHOLE_MESH -> (I think this is points)
  // Association::POINTS -> map using point mapping
  // Association::CELL_SET -> how do we map this?
  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

protected:
  vtkm::filter::FilterDataSet<Derived>& operator=(const vtkm::filter::FilterDataSet<Derived>&) =
    default;
  VTKM_CONT
  void CopyStateFrom(const FilterDataSet<Derived>* filter) { *this = *filter; }

private:
  friend class vtkm::filter::Filter<Derived>;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/FilterDataSet.hxx>

#endif // vtk_m_filter_DataSetFilter_h
