//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/rendering/raytracing/ConnectivityTracer.h>

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/rendering/raytracing/MeshConnectivityBuilder.h>

#include <vtkm/VectorAnalysis.h>


#ifndef CELL_SHAPE_ZOO
#define CELL_SHAPE_ZOO 255
#endif

#ifndef CELL_SHAPE_STRUCTURED
#define CELL_SHAPE_STRUCTURED 254
#endif

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

} //namespace detail

void ConnectivityTracer::Init()
{
  //
  // Check to see if a sample distance was set
  //
  vtkm::Bounds coordsBounds = Coords.GetBounds();
  vtkm::Float64 maxLength = 0.;
  maxLength = vtkm::Max(maxLength, coordsBounds.X.Length());
  maxLength = vtkm::Max(maxLength, coordsBounds.Y.Length());
  maxLength = vtkm::Max(maxLength, coordsBounds.Z.Length());
  BumpDistance = maxLength * BumpEpsilon;

  if (SampleDistance <= 0)
  {
    BoundingBox[0] = vtkm::Float32(coordsBounds.X.Min);
    BoundingBox[1] = vtkm::Float32(coordsBounds.X.Max);
    BoundingBox[2] = vtkm::Float32(coordsBounds.Y.Min);
    BoundingBox[3] = vtkm::Float32(coordsBounds.Y.Max);
    BoundingBox[4] = vtkm::Float32(coordsBounds.Z.Min);
    BoundingBox[5] = vtkm::Float32(coordsBounds.Z.Max);

    BackgroundColor[0] = 1.f;
    BackgroundColor[1] = 1.f;
    BackgroundColor[2] = 1.f;
    BackgroundColor[3] = 1.f;
    const vtkm::Float32 defaultSampleRate = 200.f;
    // We need to set some default sample distance
    vtkm::Vec3f_32 extent;
    extent[0] = BoundingBox[1] - BoundingBox[0];
    extent[1] = BoundingBox[3] - BoundingBox[2];
    extent[2] = BoundingBox[5] - BoundingBox[4];
    SampleDistance = vtkm::Magnitude(extent) / defaultSampleRate;
  }
}

vtkm::Id ConnectivityTracer::GetNumberOfMeshCells() const
{
  return CellSet.GetNumberOfCells();
}

void ConnectivityTracer::SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void ConnectivityTracer::SetVolumeData(const vtkm::cont::Field& scalarField,
                                       const vtkm::Range& scalarBounds,
                                       const vtkm::cont::UnknownCellSet& cellSet,
                                       const vtkm::cont::CoordinateSystem& coords)
{
  //TODO: Need a way to tell if we have been updated
  ScalarField = scalarField;
  ScalarBounds = scalarBounds;
  CellSet = cellSet;
  Coords = coords;
  MeshConnIsConstructed = false;

  const bool isSupportedField = ScalarField.IsFieldCell() || ScalarField.IsFieldPoint();
  if (!isSupportedField)
  {
    throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
  }
  FieldAssocPoints = ScalarField.IsFieldPoint();

  this->Integrator = Volume;

  if (MeshContainer == nullptr)
  {
    delete MeshContainer;
  }
  MeshConnectivityBuilder builder;
  MeshContainer = builder.BuildConnectivity(cellSet, coords);

  Locator.SetCellSet(this->CellSet);
  Locator.SetCoordinates(this->Coords);
  Locator.Update();
}

void ConnectivityTracer::SetEnergyData(const vtkm::cont::Field& absorption,
                                       const vtkm::Int32 numBins,
                                       const vtkm::cont::UnknownCellSet& cellSet,
                                       const vtkm::cont::CoordinateSystem& coords,
                                       const vtkm::cont::Field& emission)
{
  bool isSupportedField = absorption.GetAssociation() == vtkm::cont::Field::Association::Cells;
  if (!isSupportedField)
    throw vtkm::cont::ErrorBadValue("Absorption Field '" + absorption.GetName() +
                                    "' not accociated with cells");
  ScalarField = absorption;
  CellSet = cellSet;
  Coords = coords;
  MeshConnIsConstructed = false;
  // Check for emission
  HasEmission = false;

  if (emission.GetAssociation() != vtkm::cont::Field::Association::Any)
  {
    if (emission.GetAssociation() != vtkm::cont::Field::Association::Cells)
      throw vtkm::cont::ErrorBadValue("Emission Field '" + emission.GetName() +
                                      "' not accociated with cells");
    HasEmission = true;
    EmissionField = emission;
  }
  // Do some basic range checking
  if (numBins < 1)
    throw vtkm::cont::ErrorBadValue("Number of energy bins is less than 1");
  vtkm::Id binCount = ScalarField.GetNumberOfValues();
  vtkm::Id cellCount = this->GetNumberOfMeshCells();
  if (cellCount != (binCount / vtkm::Id(numBins)))
  {
    std::stringstream message;
    message << "Invalid number of absorption bins\n";
    message << "Number of cells: " << cellCount << "\n";
    message << "Number of field values: " << binCount << "\n";
    message << "Number of bins: " << numBins << "\n";
    throw vtkm::cont::ErrorBadValue(message.str());
  }
  if (HasEmission)
  {
    binCount = EmissionField.GetNumberOfValues();
    if (cellCount != (binCount / vtkm::Id(numBins)))
    {
      std::stringstream message;
      message << "Invalid number of emission bins\n";
      message << "Number of cells: " << cellCount << "\n";
      message << "Number of field values: " << binCount << "\n";
      message << "Number of bins: " << numBins << "\n";
      throw vtkm::cont::ErrorBadValue(message.str());
    }
  }
  //TODO: Need a way to tell if we have been updated
  this->Integrator = Energy;

  if (MeshContainer == nullptr)
  {
    delete MeshContainer;
  }

  MeshConnectivityBuilder builder;
  MeshContainer = builder.BuildConnectivity(cellSet, coords);
  Locator.SetCellSet(this->CellSet);
  Locator.SetCoordinates(this->Coords);
  Locator.Update();
}

void ConnectivityTracer::SetBackgroundColor(const vtkm::Vec4f_32& backgroundColor)
{
  BackgroundColor = backgroundColor;
}

void ConnectivityTracer::SetSampleDistance(const vtkm::Float32& distance)
{
  if (distance <= 0.f)
    throw vtkm::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}

void ConnectivityTracer::ResetTimers()
{
  IntersectTime = 0.;
  IntegrateTime = 0.;
  SampleTime = 0.;
  LostRayTime = 0.;
  MeshEntryTime = 0.;
}

void ConnectivityTracer::LogTimers()
{
  Logger* logger = Logger::GetInstance();
  logger->AddLogData("intersect ", IntersectTime);
  logger->AddLogData("integrate ", IntegrateTime);
  logger->AddLogData("sample_cells ", SampleTime);
  logger->AddLogData("lost_rays ", LostRayTime);
  logger->AddLogData("mesh_entry", LostRayTime);
}

}
}
} // namespace vtkm::rendering::raytracing
