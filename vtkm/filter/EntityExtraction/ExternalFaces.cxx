
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_ExternalFaces_cxx

#include <vtkm/filter/EntityExtraction/ExternalFaces.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ExternalFaces::ExternalFaces()
  : vtkm::filter::FilterDataSet<ExternalFaces>()
  , CompactPoints(false)
  , Worklet()
{
  this->SetPassPolyData(true);
}

//-----------------------------------------------------------------------------
vtkm::cont::DataSet ExternalFaces::GenerateOutput(const vtkm::cont::DataSet& input,
                                                  vtkm::cont::CellSetExplicit<>& outCellSet)
{
  //This section of ExternalFaces is independent of any input so we can build it
  //into the vtkm_filter library

  //3. Check the fields of the dataset to see what kinds of fields are present so
  //   we can free the cell mapping array if it won't be needed.
  const vtkm::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    auto f = input.GetField(fieldIdx);
    hasCellFields = f.IsFieldCell();
  }

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  //4. create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(false);
    return this->Compactor.Execute(output);
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
vtkm::cont::DataSet ExternalFaces::DoExecute(const vtkm::cont::DataSet& input)
{
  //1. extract the cell set
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  //2. using the policy convert the dynamic cell set, and run the
  // external faces worklet
  vtkm::cont::CellSetExplicit<> outCellSet;

  if (cells.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    this->Worklet.Run(cells.Cast<vtkm::cont::CellSetStructured<3>>(),
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                      outCellSet);
  }
  else
  {
    this->Worklet.Run(
      vtkm::filter::ApplyPolicyCellSetUnstructured(cells, vtkm::filter::PolicyDefault{}, *this),
      outCellSet);
  }

  return this->GenerateOutput(input, outCellSet);
}

bool ExternalFaces::MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.MapFieldOntoOutput(result, field);
    }
    else
    {
      result.AddField(field);
      return true;
    }
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, this->Worklet.GetCellIdMap(), result);
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

}
}
