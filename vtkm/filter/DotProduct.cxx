//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/DotProduct.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
class DotProduct : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T, vtkm::IdComponent Size>
  VTKM_EXEC void operator()(const vtkm::Vec<T, Size>& v1,
                            const vtkm::Vec<T, Size>& v2,
                            T& outValue) const
  {
    outValue = static_cast<T>(vtkm::Dot(v1, v2));
  }

  template <typename T>
  VTKM_EXEC void operator()(T s1, T s2, T& outValue) const
  {
    outValue = static_cast<T>(s1 * s2);
  }
};
} // namespace worklet

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

struct ResolveTypeFunctor
{
  template <typename T, typename Storage>
  void operator()(const vtkm::cont::ArrayHandle<T, Storage>& primary,
                  const DotProduct& filter,
                  const vtkm::cont::DataSet& input,
                  vtkm::cont::DataSet& outDataSet)
  {
    vtkm::cont::Field secondaryField;
    if (filter.GetUseCoordinateSystemAsSecondaryField())
    {
      secondaryField = input.GetCoordinateSystem(filter.GetSecondaryCoordinateSystemIndex());
    }
    else
    {
      secondaryField =
        input.GetField(filter.GetSecondaryFieldName(), filter.GetSecondaryFieldAssociation());
    }
    auto secondary = secondaryField.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<T, Storage>>();

    vtkm::cont::ArrayHandle<typename vtkm::VecTraits<T>::ComponentType> output;
    vtkm::cont::Invoker invoker;
    invoker(vtkm::worklet::DotProduct{}, primary, secondary, output);

    outDataSet = CreateResultFieldPoint(input, output, filter.GetOutputFieldName());
  }
};
//-----------------------------------------------------------------------------
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
  auto primary =
    firstField.GetData().ResetTypes<vtkm::TypeListFloatVec, VTKM_DEFAULT_STORAGE_LIST>();

  vtkm::cont::DataSet output;
  vtkm::cont::CastAndCall(primary, ResolveTypeFunctor{}, *this, inDataSet, output);
  return output;
}
}
} // namespace vtkm::filter
