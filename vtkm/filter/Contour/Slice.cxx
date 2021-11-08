//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Slice_cxx
#define vtk_m_filter_Slice_cxx

#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/filter/Contour/Slice.h>

namespace vtkm
{
namespace filter
{

vtkm::cont::DataSet Slice::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& coords = input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::DataSet result;
  auto impFuncEval =
    vtkm::ImplicitFunctionValueFunctor<vtkm::ImplicitFunctionGeneral>(this->Function);
  // FIXME: do we still need GetDataAsMultiplexer()? Can GetData() do it?
  auto sliceScalars =
    vtkm::cont::make_ArrayHandleTransform(coords.GetDataAsMultiplexer(), impFuncEval);
  auto field = vtkm::cont::make_FieldPoint("sliceScalars", sliceScalars);
  // input is a const, we can not AddField to it.
  vtkm::cont::DataSet clone = input;
  clone.AddField(field);

  this->Contour::SetIsoValue(0.0);
  this->Contour::SetActiveField("sliceScalars");
  result = this->Contour::DoExecute(clone);

  CallMapFieldOntoOutput(input, result);

  return result;
}

}
} // vtkm::filter

#endif // vtk_m_filter_Slice_cxx
