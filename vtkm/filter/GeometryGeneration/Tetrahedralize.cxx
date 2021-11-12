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

namespace vtkm
{
namespace filter
{
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

VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          vtkm::worklet::Tetrahedralize& worklet)
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
      worklet.GetOutCellScatter().GetOutputToInputMap();
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
} // anonymous namespace

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet Tetrahedralize::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::worklet::Tetrahedralize Worklet;
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::cont::CastAndCall(
    vtkm::filter::ApplyPolicyCellSet(cells, vtkm::filter::PolicyDefault{}, *this),
    DeduceCellSet{},
    Worklet,
    outCellSet);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  auto mapper = [&, this](auto& result, const auto& f) { DoMapField(result, f, Worklet); };
  MapFieldsOntoOutput(input, output, mapper);

  return output;
}

}
}
