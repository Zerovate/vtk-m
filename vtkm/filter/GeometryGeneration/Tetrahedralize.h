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
#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace worklet
{
class Tetrahedralize;
}
namespace filter
{

class VTKM_FILTER_GEOMETRYGENERATION_EXPORT Tetrahedralize
  : public vtkm::filter::FilterDataSet<Tetrahedralize>
{
public:
  VTKM_CONT
  Tetrahedralize();

  VTKM_CONT
  ~Tetrahedralize();

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  // Map new field onto the resulting dataset after running the filter
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    return this->MapFieldOntoOutput(result, field);
  }

private:
  std::unique_ptr<vtkm::worklet::Tetrahedralize> Worklet;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_Tetrahedralize_h
