//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_internal_ResolveFieldTypeAndMap_h
#define vtk_m_filter_internal_ResolveFieldTypeAndMap_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{
namespace internal
{

struct ResolveFieldTypeAndMap
{
  Filter* DerivedClass;
  vtkm::cont::DataSet& InputResult;
  const vtkm::filter::FieldMetadata& Metadata;
  bool& RanProperly;

  ResolveFieldTypeAndMap(Filter* derivedClass,
                         vtkm::cont::DataSet& inResult,
                         const vtkm::filter::FieldMetadata& fieldMeta,
                         bool& ran)
    : DerivedClass(derivedClass)
    , InputResult(inResult)
    , Metadata(fieldMeta)
    , RanProperly(ran)
  {
  }

  template <typename T, typename StorageTag>
  void operator()(const vtkm::cont::ArrayHandle<T, StorageTag>&) const
  {
    //    this->RanProperly =
    //      this->DerivedClass->DoMapField(this->InputResult, field, this->Metadata, vtkm::filter::PolicyDefault{});
  }

private:
  void operator=(const ResolveFieldTypeAndMap&) = delete;
};
}
}
} // namespace vtkm::filter::internal

#endif //vtk_m_filter_internal_ResolveFieldTypeAndMap_h
