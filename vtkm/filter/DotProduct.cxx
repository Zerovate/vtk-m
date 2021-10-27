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
                  vtkm::cont::UnknownArrayHandle& output) const
  {
    const auto& secondaryField = [&]() -> const vtkm::cont::Field& {
      if (filter.GetUseCoordinateSystemAsSecondaryField())
      {
        return input.GetCoordinateSystem(filter.GetSecondaryCoordinateSystemIndex());
      }
      else
      {
        return input.GetField(filter.GetSecondaryFieldName(),
                              filter.GetSecondaryFieldAssociation());
      }
    }();

    vtkm::cont::UnknownArrayHandle secondary = vtkm::cont::ArrayHandle<T>{};
    secondary.CopyShallowIfPossible(secondaryField.GetData());

    vtkm::cont::ArrayHandle<typename vtkm::VecTraits<T>::ComponentType> result;
    vtkm::cont::Invoker invoker;
    invoker(vtkm::worklet::DotProduct{},
            primary,
            secondary.template AsArrayHandle<vtkm::cont::ArrayHandle<T>>(),
            result);
    output = result;
  }
};
//-----------------------------------------------------------------------------
VTKM_CONT_EXPORT vtkm::cont::DataSet DotProduct::DoExecute(
  const vtkm::cont::DataSet& inDataSet) const
{
  const auto& primary = this->GetFieldFromDataSet(inDataSet).GetData();

  vtkm::cont::UnknownArrayHandle outArray;
  primary.CastAndCallForTypesWithFloatFallback<VTKM_DEFAULT_TYPE_LIST, VTKM_DEFAULT_STORAGE_LIST>(
    ResolveTypeFunctor{}, *this, inDataSet, outArray);

  vtkm::cont::DataSet outDataSet = inDataSet; // copy
  outDataSet.AddField({ this->GetOutputFieldName(),
                        this->GetFieldFromDataSet(inDataSet).GetAssociation(),
                        outArray });
  return outDataSet;
}
}
} // namespace vtkm::filter
