//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExternalFaces_h
#define vtk_m_filter_ExternalFaces_h

#include <vtkm/filter/CleanGrid/CleanGrid.h>
#include <vtkm/filter/EntityExtraction/vtkm_filter_entityextraction_export.h>
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace worklet
{
struct ExternalFaces;
}
namespace filter
{

/// \brief  Extract external faces of a geometry
///
/// ExternalFaces is a filter that extracts all external faces from a
/// data set. An external face is defined is defined as a face/side of a cell
/// that belongs only to one cell in the entire mesh.
/// @warning
/// This filter is currently only supports propagation of point properties
///
// FIXME: when did I make it a Filter rahter than FilterDataSet?
class VTKM_FILTER_ENTITYEXTRACTION_EXPORT ExternalFaces : public vtkm::filter::Filter
{
public:
  ExternalFaces();
  ~ExternalFaces();

  // New Design: I am too lazy to make this filter thread-safe. Let's use it as a example of
  // thread un-safe filter.
  bool CanThread() const override { return false; }

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the
  // unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  // When PassPolyData is set (the default), incoming poly data (0D, 1D, and 2D cells)
  // will be passed to the output external faces data set.
  VTKM_CONT
  bool GetPassPolyData() const { return this->PassPolyData; }
  VTKM_CONT
  void SetPassPolyData(bool value);

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field);

private:
  bool CompactPoints;
  bool PassPolyData;

  vtkm::cont::DataSet GenerateOutput(const vtkm::cont::DataSet& input,
                                     vtkm::cont::CellSetExplicit<>& outCellSet);

  vtkm::filter::CleanGrid Compactor;
  std::unique_ptr<vtkm::worklet::ExternalFaces> Worklet;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_ExternalFaces_h
