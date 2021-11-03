//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/GeometryGeneration/Tetrahedralize.h>
#include <vtkm/filter/GeometryGeneration/worklet/Tetrahedralize.h>
#include <vtkm/filter/MapFieldPermutation.h>

namespace
{
struct DeduceCellSet
{
  template <typename CellSetType>
  void operator()(const CellSetType& cellset,
                  vtkm::worklet::Tetrahedralize& worklet,
                  vtkm::cont::CellSetSingleType<>& outCellSet) const
  {
    outCellSet = worklet.Run(cellset);
  }
};
}

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
VTKM_CONT Tetrahedralize::Tetrahedralize()
  : vtkm::filter::FilterDataSet()
  , Worklet(std::make_unique<vtkm::worklet::Tetrahedralize>())
{
}

VTKM_CONT Tetrahedralize::~Tetrahedralize() = default;

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet Tetrahedralize::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::cont::CastAndCall(
    vtkm::filter::ApplyPolicyCellSet(cells, vtkm::filter::PolicyDefault{}, *this),
    DeduceCellSet{},
    *(this->Worklet),
    outCellSet);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  CallMapFieldOntoOutput(input, output, vtkm::filter::PolicyDefault{});

  return output;
}

VTKM_CONT bool Tetrahedralize::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                  const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint())
  {
    // point data is copied as is because it was not collapsed
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldCell())
  {
    // cell data must be scattered to the cells created per input cell
    vtkm::cont::ArrayHandle<vtkm::Id> permutation =
      this->Worklet->GetOutCellScatter().GetOutputToInputMap();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
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
