//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ClipWithField_hxx
#define vtk_m_filter_ClipWithField_hxx

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/filter/Contour/ClipWithField.h>
#include <vtkm/filter/Contour/worklet/Clip.h>

namespace vtkm
{
namespace filter
{
namespace
{
struct ClipWithFieldProcessCoords
{
  template <typename T, typename Storage>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, Storage>& inCoords,
                            const std::string& coordsName,
                            const vtkm::worklet::Clip& worklet,
                            vtkm::cont::DataSet& output) const
  {
    vtkm::cont::ArrayHandle<T> outArray = worklet.ProcessPointField(inCoords);
    vtkm::cont::CoordinateSystem outCoords(coordsName, outArray);
    output.AddCoordinateSystem(outCoords);
  }
};

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                vtkm::worklet::Clip& worklet)
{
  if (field.IsFieldPoint())
  {
    auto array = vtkm::filter::ApplyPolicyFieldNotActive(field, vtkm::filter::PolicyDefault{});

    auto functor = [&](auto concrete) {
      using T = typename decltype(concrete)::ValueType;
      vtkm::cont::ArrayHandle<T> output;
      output = worklet.ProcessPointField(concrete);
      result.template AddPointField(field.GetName(), output);
    };

    array.CastAndCallWithFloatFallback(functor);
    return true;
  }
  else if (field.IsFieldCell())
  {
    // Use the precompiled field permutation function.
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = worklet.GetCellMapOutputToInput();
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
} // anonymous

//-----------------------------------------------------------------------------
vtkm::cont::DataSet ClipWithField::DoExecute(const vtkm::cont::DataSet& input)
{
  auto field = this->GetFieldFromDataSet(input);
  if (!field.IsFieldPoint())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  vtkm::worklet::Clip Worklet;

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const auto policy = vtkm::filter::PolicyDefault{};
  const auto inArray = vtkm::filter::ApplyPolicyFieldActive(
    field, policy, vtkm::filter::FilterTraits<ClipWithField>());
  vtkm::cont::DataSet output;

  auto ResolveFieldType = [&, this](auto concrete) {
    vtkm::cont::CellSetExplicit<> outputCellSet =
      Worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy, *this),
                  concrete,
                  this->ClipValue,
                  this->Invert);

    output.SetCellSet(outputCellSet);

    // Compute the new boundary points and add them to the output:
    for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
         ++coordSystemId)
    {
      vtkm::cont::CoordinateSystem coords = input.GetCoordinateSystem(coordSystemId);
      coords.GetData().CastAndCall(ClipWithFieldProcessCoords{}, coords.GetName(), Worklet, output);
    }
  };

  inArray.CastAndCallWithFloatFallback(ResolveFieldType);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, Worklet); };
  MapFieldsOntoOutput(input, output, mapper);

  return output;
}

}
} // end namespace vtkm::filter

#endif
