//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/filter/DotProduct.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
VTKM_CONT_EXPORT DotProduct::DotProduct()
  : vtkm::filter::FilterField<DotProduct>()
  , SecondaryFieldName()
  , SecondaryFieldAssociation(vtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsSecondaryField(false)
  , SecondaryCoordinateSystemIndex(0)
{
  this->SetOutputFieldName("dotproduct");
}

//-----------------------------------------------------------------------------
//template <typename T, typename StorageType, typename DerivedPolicy>
VTKM_CONT_EXPORT vtkm::cont::DataSet DotProduct::DoExecute(
  const vtkm::cont::DataSet& inDataSet) const
{
  vtkm::cont::Field firstField;

  if (this->GetUseCoordinateSystemAsPrimaryField())
  {
    firstField = inDataSet.GetCoordinateSystem(this->GetPrimaryCoordinateSystemIndex());
  }
  else
  {
    firstField = inDataSet.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
  }
  auto primary = firstField.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();

  vtkm::cont::Field secondaryField;
  if (this->UseCoordinateSystemAsSecondaryField)
  {
    secondaryField = inDataSet.GetCoordinateSystem(this->GetSecondaryCoordinateSystemIndex());
  }
  else
  {
    secondaryField = inDataSet.GetField(this->SecondaryFieldName, this->SecondaryFieldAssociation);
  }
  //  auto secondary = vtkm::filter::ApplyPolicyFieldOfType<T>(secondaryField, policy, *this);
  auto secondary = secondaryField.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();

  //  vtkm::cont::ArrayHandle<typename vtkm::VecTraits<T>::ComponentType> output;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> output;
  this->Invoke(vtkm::worklet::DotProduct{}, primary, secondary, output);

  // Assume the field associated should be the same as the first field. The second field is
  // also assumed to have the same association as the first.
  return CreateResult(
    inDataSet, output, this->GetOutputFieldName(), vtkm::filter::FieldMetadata{ firstField });
}
}
} // namespace vtkm::filter
