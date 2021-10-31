//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/filter/Contour/Contour.h>
#include <vtkm/worklet/SurfaceNormals.h>

namespace vtkm
{
namespace filter
{

namespace
{

template <typename CellSetList>
inline bool IsCellSetStructured(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellset)
{
  if (cellset.template IsType<vtkm::cont::CellSetStructured<1>>() ||
      cellset.template IsType<vtkm::cont::CellSetStructured<2>>() ||
      cellset.template IsType<vtkm::cont::CellSetStructured<3>>())
  {
    return true;
  }
  return false;
}
} // anonymous namespace

//-----------------------------------------------------------------------------
Contour::Contour()
  : vtkm::filter::FilterDataSetWithField<Contour>()
  , IsoValues()
  , GenerateNormals(false)
  , AddInterpolationEdgeIds(false)
  , ComputeFastNormalsForStructured(false)
  , ComputeFastNormalsForUnstructured(true)
  , NormalArrayName("normals")
  , InterpolationEdgeIdsArrayName("edgeIds")
  , Worklet()
{
  // todo: keep an instance of marching cubes worklet as a member variable
}

//-----------------------------------------------------------------------------
vtkm::cont::DataSet Contour::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  // ApplyPolicyFeildActive turns the UnknownArrayHandle to UncerntainArrayHandle with
  // certain ValueType and Stroage based on PolicyDefault and Filter::Supported type. We
  // could just do it ourselves but here we are demonstrating what the "helper" function
  // looks like.
  if (!this->GetFieldFromDataSet(inDataSet).IsFieldPoint())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->IsoValues.empty())
  {
    throw vtkm::cont::ErrorFilterExecution("No iso-values provided.");
  }

  const auto policy = vtkm::filter::PolicyDefault{};

  const auto& field = vtkm::filter::ApplyPolicyFieldActive(
    this->GetFieldFromDataSet(inDataSet), policy, vtkm::filter::FilterTraits<Contour>());

  // Check the fields of the dataset to see what kinds of fields are present so
  // we can free the mapping arrays that won't be needed. A point field must
  // exist for this algorithm, so just check cells.
  const vtkm::Id numFields = inDataSet.GetNumberOfFields();
  bool hasCellFields = false;
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    const auto& f = inDataSet.GetField(fieldIdx);
    hasCellFields = f.IsFieldCell();
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = inDataSet.GetCellSet();

  const vtkm::cont::CoordinateSystem& coords =
    inDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using Vec3HandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  vtkm::cont::DataSet output;
  vtkm::cont::CellSetSingleType<> outputCells;

  // TODO: what is the following comments about? Is it still relevant?
  //not sold on this as we have to generate more signatures for the
  //worklet with the design
  //But I think we should get this to compile before we tinker with
  //a more efficient api

  bool generateHighQualityNormals = IsCellSetStructured(cells)
    ? !this->ComputeFastNormalsForStructured
    : !this->ComputeFastNormalsForUnstructured;

  // We are using a C++14 auto lambda here. The advantage over a Functor is obvious, we don't
  // need to explicitly passing filter, input/output DataSets etc. thus reduce the impact to
  // the original code. Due to the API of CastAndCall the lambda still need to take the "target"
  // field as a parameter in order to generate various instantiations for the lambda. One downside
  // is that now we loose the ability of explicit instantiation of a template.
  // On the other hand, we can instead instantiate the Worklet.Run() but I don't know how to update
  // the auto-explicit-instantiation facility.
  auto ResolveFieldType = [&, this](auto concrete) {
    std::vector<typename decltype(concrete)::ValueType> ivalues(IsoValues.begin(), IsoValues.end());

    if (this->GenerateNormals && generateHighQualityNormals)
    {
      outputCells = this->Worklet.Run(ivalues,
                                      vtkm::filter::ApplyPolicyCellSet(cells, policy, *this),
                                      coords.GetData(),
                                      concrete,
                                      vertices,
                                      normals);
    }
    else
    {
      outputCells = this->Worklet.Run(ivalues,
                                      vtkm::filter::ApplyPolicyCellSet(cells, policy, *this),
                                      coords.GetData(),
                                      concrete,
                                      vertices);
    }
  };

  field.CastAndCall(ResolveFieldType);

  if (this->GenerateNormals)
  {
    if (!generateHighQualityNormals)
    {
      Vec3HandleType faceNormals;
      vtkm::worklet::FacetedSurfaceNormals faceted;
      faceted.Run(outputCells, vertices, faceNormals);

      vtkm::worklet::SmoothSurfaceNormals smooth;
      smooth.Run(outputCells, faceNormals, normals);
    }

    output.AddField(vtkm::cont::make_FieldPoint(this->NormalArrayName, normals));
  }

  if (this->AddInterpolationEdgeIds)
  {
    vtkm::cont::Field interpolationEdgeIdsField(InterpolationEdgeIdsArrayName,
                                                vtkm::cont::Field::Association::POINTS,
                                                this->Worklet.GetInterpolationEdgeIds());
    output.AddField(interpolationEdgeIdsField);
  }

  //assign the connectivity to the cell set
  output.SetCellSet(outputCells);

  //add the coordinates to the output dataset
  vtkm::cont::CoordinateSystem outputCoords("coordinates", vertices);
  output.AddCoordinateSystem(outputCoords);

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  return output;
}
}
}
