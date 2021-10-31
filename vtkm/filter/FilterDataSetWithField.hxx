//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/FilterTraits.h>
#include <vtkm/filter/PolicyDefault.h>

#include <vtkm/filter/internal/ResolveFieldTypeAndMap.h>

#include <vtkm/cont/Error.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm
{
namespace filter
{

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT FilterDataSetWithField<Derived>::FilterDataSetWithField()
  : OutputFieldName()
  , CoordinateSystemIndex(0)
  , ActiveFieldName()
  , ActiveFieldAssociation(vtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsField(false)
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT FilterDataSetWithField<Derived>::~FilterDataSetWithField()
{
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT bool FilterDataSetWithField<Derived>::MapFieldOntoOutput(
  vtkm::cont::DataSet& result,
  const vtkm::cont::Field& field,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  bool valid = false;

  vtkm::filter::FieldMetadata metaData(field);
  using FunctorType = internal::ResolveFieldTypeAndMap<Derived, DerivedPolicy>;
  FunctorType functor(static_cast<Derived*>(this), result, metaData, policy, valid);

  vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicyFieldNotActive(field, policy), functor);

  //the bool valid will be modified by the map algorithm to hold if the
  //mapping occurred or not. If the mapping was good a new field has been
  //added to the result that was passed in.
  return valid;
}
}
}
