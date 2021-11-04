//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Contour_h
#define vtk_m_filter_Contour_h

#include <vtkm/filter/Contour/vtkm_filter_contour_export.h>
#include <vtkm/filter/Contour/worklet/Contour.h>
#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/filter/MapFieldPermutation.h>

#include <vtkm/filter/Instantiations.h>

namespace vtkm
{
namespace filter
{
/// \brief generate isosurface(s) from a Volume

/// Takes as input a volume (e.g., 3D structured point set) and generates on
/// output one or more isosurfaces.
/// Multiple contour values must be specified to generate the isosurfaces.
/// @warning
/// This filter is currently only supports 3D volumes.
class VTKM_FILTER_CONTOUR_EXPORT Contour : public vtkm::filter::FilterDataSetWithField
{
public:
  using SupportedTypes = vtkm::List<vtkm::UInt8, vtkm::Int8, vtkm::Float32, vtkm::Float64>;

  VTKM_CONT
  Filter* Clone() const override
  {
    Contour* clone = new Contour();
    clone->CopyStateFrom(this);
    return clone;
  }

  VTKM_CONT
  bool CanThread() const override { return true; }

  Contour();

  void SetNumberOfIsoValues(vtkm::Id num)
  {
    if (num >= 0)
    {
      this->IsoValues.resize(static_cast<std::size_t>(num));
    }
  }

  vtkm::Id GetNumberOfIsoValues() const { return static_cast<vtkm::Id>(this->IsoValues.size()); }

  void SetIsoValue(vtkm::Float64 v) { this->SetIsoValue(0, v); }

  void SetIsoValue(vtkm::Id index, vtkm::Float64 v)
  {
    std::size_t i = static_cast<std::size_t>(index);
    if (i >= this->IsoValues.size())
    {
      this->IsoValues.resize(i + 1);
    }
    this->IsoValues[i] = v;
  }

  void SetIsoValues(const std::vector<vtkm::Float64>& values) { this->IsoValues = values; }

  vtkm::Float64 GetIsoValue(vtkm::Id index) const
  {
    return this->IsoValues[static_cast<std::size_t>(index)];
  }

  /// Set/Get whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together. Duplicate points are identified
  /// by the unique edge it was generated from.
  ///
  VTKM_CONT
  void SetMergeDuplicatePoints(bool on) { this->Worklet.SetMergeDuplicatePoints(on); }

  VTKM_CONT
  bool GetMergeDuplicatePoints() const { return this->Worklet.GetMergeDuplicatePoints(); }

  /// Set/Get whether normals should be generated. Off by default. If enabled,
  /// the default behaviour is to generate high quality normals for structured
  /// datasets, using gradients, and generate fast normals for unstructured
  /// datasets based on the result triangle mesh.
  ///
  VTKM_CONT
  void SetGenerateNormals(bool on) { this->GenerateNormals = on; }
  VTKM_CONT
  bool GetGenerateNormals() const { return this->GenerateNormals; }

  /// Set/Get whether to append the ids of the intersected edges to the vertices of the isosurface triangles. Off by default.
  VTKM_CONT
  void SetAddInterpolationEdgeIds(bool on) { this->AddInterpolationEdgeIds = on; }
  VTKM_CONT
  bool GetAddInterpolationEdgeIds() const { return this->AddInterpolationEdgeIds; }

  /// Set/Get whether the fast path should be used for normals computation for
  /// structured datasets. Off by default.
  VTKM_CONT
  void SetComputeFastNormalsForStructured(bool on) { this->ComputeFastNormalsForStructured = on; }
  VTKM_CONT
  bool GetComputeFastNormalsForStructured() const { return this->ComputeFastNormalsForStructured; }

  /// Set/Get whether the fast path should be used for normals computation for
  /// unstructured datasets. On by default.
  VTKM_CONT
  void SetComputeFastNormalsForUnstructured(bool on)
  {
    this->ComputeFastNormalsForUnstructured = on;
  }
  VTKM_CONT
  bool GetComputeFastNormalsForUnstructured() const
  {
    return this->ComputeFastNormalsForUnstructured;
  }

  VTKM_CONT
  void SetNormalArrayName(const std::string& name) { this->NormalArrayName = name; }

  VTKM_CONT
  const std::string& GetNormalArrayName() const { return this->NormalArrayName; }

  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& concrete) override;

  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field)
  {
    if (field.IsFieldPoint())
    {
      // If the field is a point field, then we need to do a custom interpolation of the points.
      // In this case, we need to call the superclass's MapFieldOntoOutput, which will in turn
      // call our DoMapField.
      //      return this->FilterDataSetWithField::MapFieldOntoOutput(result, field);
      auto array = vtkm::filter::ApplyPolicyFieldNotActive(field, vtkm::filter::PolicyDefault{});

      auto functor = [&, this](auto concrete) {
        auto fieldArray = this->Worklet.template ProcessPointField(concrete);
        result.template AddPointField(field.GetName(), fieldArray);
      };
      array.CastAndCallWithFloatFallback(functor);
      return true;
    }
    else if (field.IsFieldCell())
    {
      // Use the precompiled field permutation function.
      vtkm::cont::ArrayHandle<vtkm::Id> permutation = this->Worklet.GetCellIdMap();
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

protected:
  VTKM_CONT
  void CopyStateFrom(const Contour* contour)
  {
    this->FilterDataSetWithField::CopyStateFrom(contour);

    this->IsoValues = contour->IsoValues;
    this->GenerateNormals = contour->GenerateNormals;
    this->AddInterpolationEdgeIds = contour->AddInterpolationEdgeIds;
    this->ComputeFastNormalsForStructured = contour->ComputeFastNormalsForStructured;
    this->ComputeFastNormalsForUnstructured = contour->ComputeFastNormalsForUnstructured;
    this->NormalArrayName = contour->NormalArrayName;
    this->InterpolationEdgeIdsArrayName = contour->InterpolationEdgeIdsArrayName;
  }

private:
  std::vector<vtkm::Float64> IsoValues;
  bool GenerateNormals;
  bool AddInterpolationEdgeIds;
  bool ComputeFastNormalsForStructured;
  bool ComputeFastNormalsForUnstructured;
  std::string NormalArrayName;
  std::string InterpolationEdgeIdsArrayName;
  vtkm::worklet::Contour Worklet;
};

}
} // namespace vtkm::filter

#endif // vtk_m_filter_Contour_h
