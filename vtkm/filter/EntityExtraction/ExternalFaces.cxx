
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
#include <vtkm/filter/EntityExtraction/worklet/ExternalFaces.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ExternalFaces::ExternalFaces()
  : CompactPoints(false)
  , Worklet(std::make_unique<vtkm::worklet::ExternalFaces>())
{
  this->SetPassPolyData(true);
}

ExternalFaces::~ExternalFaces() = default;

//-----------------------------------------------------------------------------
void ExternalFaces::SetPassPolyData(bool value)
{
  this->PassPolyData = value;
  this->Worklet->SetPassPolyData(value);
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
    this->Worklet->ReleaseCellMapArrays();
  }

  //4. create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  return output;
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
    this->Worklet->Run(cells.Cast<vtkm::cont::CellSetStructured<3>>(),
                       input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                       outCellSet);
  }
  else
  {
    this->Worklet->Run(
      vtkm::filter::ApplyPolicyCellSetUnstructured(cells, vtkm::filter::PolicyDefault{}, *this),
      outCellSet);
  }

  // New Design: we generate new output and map the fields first.
  auto output = this->GenerateOutput(input, outCellSet);
  auto mapper = [&, this](auto& result, const auto& f) {
    // New Design: We are still using the old MapFieldOntoOutput to demonstrate the transition
    this->MapFieldOntoOutput(result, f);
  };

  MapFieldsOntoOutput(input, output, mapper);

  // New Design: then we remove entities if requested.
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

// TODO: Can we make this something like "trivialMapper" for FilterDataSet?
bool ExternalFaces::MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, this->Worklet->GetCellIdMap(), result);
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
