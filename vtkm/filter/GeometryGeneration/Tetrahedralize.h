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

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/filter/GeometryGeneration/vtkm_filter_geometrygeneration_export.h>
#include <vtkm/filter/GeometryGeneration/worklet/Tetrahedralize.h>
#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace filter
{

class VTKM_FILTER_GEOMETRYGENERATION_EXPORT Tetrahedralize
  : public vtkm::filter::FilterDataSet<Tetrahedralize>
{
public:
  VTKM_CONT
  Tetrahedralize();

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input);

  // Map new field onto the resulting dataset after running the filter
  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    if (field.IsFieldPoint())
    {
      // point data is copied as is because it was not collapsed
      result.AddField(field);
      return true;
    }
    else if (field.IsFieldCell())
    {
      // cell data must be scattered to the cells created per input cell
      vtkm::cont::ArrayHandle<vtkm::Id> permutation =
        this->Worklet.GetOutCellScatter().GetOutputToInputMap();
      return vtkm::filter::MapFieldPermutation(field, permutation, result);
    }
    else if (field.IsFieldGlobal())
    {
      result.AddField(field);
      return true;
    }
    else
    {
      return false;
    }
  }

private:
  vtkm::worklet::Tetrahedralize Worklet;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_Tetrahedralize_h
