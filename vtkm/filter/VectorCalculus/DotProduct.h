//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DotProduct_h
#define vtk_m_filter_DotProduct_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/VectorCalculus/vtkm_filter_vectorcalculus_export.h>

namespace vtkm
{
namespace filter
{

class VTKM_FILTER_VECTORCALCULUS_EXPORT DotProduct : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  DotProduct();

  //@{
  /// Choose the primary field to operate on. In the cross product operation A x B, A is
  /// the primary field.
  VTKM_CONT
  void SetPrimaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->SetActiveField(name, association);
  }

  VTKM_CONT const std::string& GetPrimaryFieldName() const { return this->GetActiveFieldName(); }
  VTKM_CONT vtkm::cont::Field::Association GetPrimaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation();
  }
  //@}

  //@{
  /// When set to true, uses a coordinate system as the primary field instead of the one selected
  /// by name. Use SetPrimaryCoordinateSystem to select which coordinate system.
  VTKM_CONT
  void SetUseCoordinateSystemAsPrimaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(flag);
  }
  VTKM_CONT
  bool GetUseCoordinateSystemAsPrimaryField() const
  {
    return this->GetUseCoordinateSystemAsField();
  }
  //@}

  //@{
  /// Select the coordinate system index to use as the primary field. This only has an effect when
  /// UseCoordinateSystemAsPrimaryField is true.
  VTKM_CONT
  void SetPrimaryCoordinateSystem(vtkm::Id index) { this->SetActiveCoordinateSystem(index); }
  VTKM_CONT
  vtkm::Id GetPrimaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex();
  }
  //@}

  //@{
  /// Choose the secondary field to operate on. In the cross product operation A x B, B is
  /// the secondary field.
  VTKM_CONT
  void SetSecondaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->SecondaryFieldName = name;
    this->SecondaryFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetSecondaryFieldName() const { return this->SecondaryFieldName; }
  VTKM_CONT vtkm::cont::Field::Association GetSecondaryFieldAssociation() const
  {
    return this->SecondaryFieldAssociation;
  }
  //@}

  //@{
  /// When set to true, uses a coordinate system as the primary field instead of the one selected
  /// by name. Use SetPrimaryCoordinateSystem to select which coordinate system.
  VTKM_CONT
  void SetUseCoordinateSystemAsSecondaryField(bool flag)
  {
    this->UseCoordinateSystemAsSecondaryField = flag;
  }
  VTKM_CONT
  bool GetUseCoordinateSystemAsSecondaryField() const
  {
    return this->UseCoordinateSystemAsSecondaryField;
  }
  //@}

  //@{
  /// Select the coordinate system index to use as the primary field. This only has an effect when
  /// UseCoordinateSystemAsPrimaryField is true.
  VTKM_CONT
  void SetSecondaryCoordinateSystem(vtkm::Id index)
  {
    this->SecondaryCoordinateSystemIndex = index;
  }
  VTKM_CONT
  vtkm::Id GetSecondaryCoordinateSystemIndex() const
  {
    return this->SecondaryCoordinateSystemIndex;
  }
  //@}

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

private:
  std::string SecondaryFieldName;
  vtkm::cont::Field::Association SecondaryFieldAssociation;
  bool UseCoordinateSystemAsSecondaryField;
  vtkm::Id SecondaryCoordinateSystemIndex;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_DotProduct_h
