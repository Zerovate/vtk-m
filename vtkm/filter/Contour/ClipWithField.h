//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ClipWithField_h
#define vtk_m_filter_ClipWithField_h

#include <vtkm/filter/Contour/vtkm_filter_contour_export.h>

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace worklet
{
class Clip;
}
namespace filter
{
/// \brief Clip a dataset using a field
///
/// Clip a dataset using a given field value. All points that are less than that
/// value are considered outside, and will be discarded. All points that are greater
/// are kept.
/// The resulting geometry will not be water tight.
class VTKM_FILTER_CONTOUR_EXPORT ClipWithField : public vtkm::filter::FilterDataSetWithField
{
public:
  using SupportedTypes = vtkm::TypeListScalarAll;

  VTKM_CONT
  void SetClipValue(vtkm::Float64 value) { this->ClipValue = value; }

  VTKM_CONT
  void SetInvertClip(bool invert) { this->Invert = invert; }

  VTKM_CONT
  vtkm::Float64 GetClipValue() const { return this->ClipValue; }

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input);

private:
  VTKM_CONT static bool DoMapField(vtkm::cont::DataSet& result,
                                   const vtkm::cont::Field& field,
                                   vtkm::worklet::Clip& worklet);
  vtkm::Float64 ClipValue = 0;
  bool Invert = false;
};

}
} // namespace vtkm::filter

#endif // vtk_m_filter_ClipWithField_h
