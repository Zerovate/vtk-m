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

namespace // anonymous namespace making worklet::DotProduct internal to this .cxx
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
} // anonymous namespace

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

    // The ApplyPolicyFieldOfType uses ArrayHandleMultiplexer which creates an enormous
    // amount of instantiations which I believe are not actually used. We should settle
    // with UnknownArrayHandle::CopyShallowIfPossible() to extract secondary field for
    // the moment.
    //  auto secondary = vtkm::filter::ApplyPolicyFieldOfType<T>(secondaryField,
    //                                                           vtkm::filter::PolicyDefault{},
    //                                                           filter);

    vtkm::cont::ArrayHandle<typename vtkm::VecTraits<T>::ComponentType> result;
    vtkm::cont::Invoker invoker;
    invoker(::worklet::DotProduct{},
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
  // ApplyPolicyFeildActive turns the UnknownArrayHandle to UncerntainArrayHandle  wich
  // certain ValueType and Stroage based on PolicyDefault and Filter::Supported btype. We
  // could just do it ourselves but here we are demonstrating what the "helper" function
  // looks like.
  const auto& primary =
    vtkm::filter::ApplyPolicyFieldActive(this->GetFieldFromDataSet(inDataSet),
                                         vtkm::filter::PolicyDefault{},
                                         vtkm::filter::FilterTraits<DotProduct>());

  vtkm::cont::UnknownArrayHandle outArray;
  primary.CastAndCallWithFloatFallback(ResolveTypeFunctor{}, *this, inDataSet, outArray);

  vtkm::cont::DataSet outDataSet = inDataSet; // copy
  outDataSet.AddField({ this->GetOutputFieldName(),
                        this->GetFieldFromDataSet(inDataSet).GetAssociation(),
                        outArray });
  return outDataSet;
}
}
} // namespace vtkm::filter
