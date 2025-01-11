//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperVolume.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/VolumeRendererStructured.h>

#include <sstream>

#define DEFAULT_SAMPLE_DISTANCE -1.f

namespace vtkm
{
namespace rendering
{

struct MapperVolume::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::Float32 SampleDistance;
  bool CompositeBackground;

  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , SampleDistance(DEFAULT_SAMPLE_DISTANCE)
    , CompositeBackground(true)
  {
  }
};

MapperVolume::MapperVolume()
  : Internals(new InternalsType)
{
}

MapperVolume::~MapperVolume() {}

void MapperVolume::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);

    if (this->Internals->Canvas == nullptr)
    {
      throw vtkm::cont::ErrorBadValue("Ray Tracer: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

vtkm::rendering::Canvas* MapperVolume::GetCanvas() const
{
  return this->Internals->Canvas;
}

template <typename CoordType>
void MapperVolume::RenderCellsImplWithCoordType(const vtkm::cont::UnknownCellSet& cellset,
                                   const vtkm::cont::CoordinateSystem& coords,
                                   const vtkm::cont::Field& scalarField,
                                   const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                                   const vtkm::rendering::Camera& camera,
                                   const vtkm::Range& scalarRange)
{
  bool isStructured = cellset.CanConvert<vtkm::cont::CellSetStructured<3>>();
  if (!isStructured)
  {
    // only care about single type and 3D
    if (!cellset.CanConvert<vtkm::cont::CellSetSingleType<>>())
    {
      std::stringstream msg;
      msg << "Mapper volume: only SingleType is supported" << std::endl;
      throw vtkm::cont::ErrorBadValue(msg.str());
    }
    if (cellset.AsCellSet<vtkm::cont::CellSetSingleType<>>().GetNumberOfPointsInCell(0) > 10)
    {
      std::stringstream msg;
      msg << "The cell has more than 10 points.\n";
      msg << "This is currently not supported by the code.\n";
      msg << "To resolve this:\n";
      msg << "   1. Enlarge the \"fieldValues\" vector in "
             "\"vtkm/rendering/raytracing/VolumeRendererUnstructured.cxx\"\n";
      msg << "   2. Change the IF condition in MapperVolume::RenderCells in "
             "\"vtkm/rendering/MapperVolume.cxx\""
          << std::endl;
      throw vtkm::cont::ErrorBadValue(msg.str());
    }
  }
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_volume");
  vtkm::cont::Timer tot_timer;
  tot_timer.Start();
  vtkm::cont::Timer timer;

  vtkm::rendering::raytracing::VolumeRendererStructured<CoordType> tracer;

  vtkm::rendering::raytracing::Camera rayCamera;
  vtkm::Int32 width = (vtkm::Int32)this->Internals->Canvas->GetWidth();
  vtkm::Int32 height = (vtkm::Int32)this->Internals->Canvas->GetHeight();
  rayCamera.SetParameters(camera, width, height);

  vtkm::rendering::raytracing::Ray<vtkm::Float32> rays;
  rayCamera.CreateRays(rays, coords.GetBounds());
  rays.Buffers.at(0).InitConst(0.f);
  raytracing::RayOperations::MapCanvasToRays(rays, camera, *this->Internals->Canvas);


  if (this->Internals->SampleDistance != DEFAULT_SAMPLE_DISTANCE)
  {
    tracer.SetSampleDistance(this->Internals->SampleDistance);
  }
  if (isStructured)
  {
    tracer.SetData(
      coords, scalarField, cellset.AsCellSet<vtkm::cont::CellSetStructured<3>>(), scalarRange);
  }
  else
  {
    const vtkm::FloatDefault L1 = -1;
    const vtkm::FloatDefault L2 = -1;
    tracer.SetData(coords, scalarField, cellset.AsCellSet<vtkm::cont::CellSetSingleType<>>(), scalarRange, L1, L2);
  }
  tracer.SetColorMap(this->ColorMap);

  tracer.Render(rays);

  timer.Start();
  this->Internals->Canvas->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);

  if (this->Internals->CompositeBackground)
  {
    this->Internals->Canvas->BlendBackground();
  }
  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);

}

void MapperVolume::RenderCellsImpl(const vtkm::cont::UnknownCellSet& cellset,
                                   const vtkm::cont::CoordinateSystem& coords,
                                   const vtkm::cont::Field& scalarField,
                                   const vtkm::cont::ColorTable& colorTable,
                                   const vtkm::rendering::Camera& camera,
                                   const vtkm::Range& scalarRange,
                                   const vtkm::cont::Field& vtkmNotUsed(ghostField))
{
  vtkm::IdComponent index = coords.GetDataAsMultiplexer().GetArrayHandleVariant().GetIndex();
  switch (index)
  {
    case 0:
      RenderCellsImplWithCoordType<vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>, vtkm::cont::StorageTagBasic>>(cellset, coords, scalarField, colorTable, camera, scalarRange);
      break;
    case 1:
      RenderCellsImplWithCoordType<vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>, vtkm::cont::StorageTagSOA>>(cellset, coords, scalarField, colorTable, camera, scalarRange);
      break;
    case 2:
      RenderCellsImplWithCoordType<vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>, vtkm::cont::StorageTagUniformPoints>>(cellset, coords, scalarField, colorTable, camera, scalarRange);
      break;
    case 3:
      RenderCellsImplWithCoordType<vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>, vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic, vtkm::cont::StorageTagBasic, vtkm::cont::StorageTagBasic>>>(cellset, coords, scalarField, colorTable, camera, scalarRange);
      break;
    case 4:
      RenderCellsImplWithCoordType<vtkm::cont::ArrayHandleCast<vtkm::Vec<float, 3>, vtkm::cont::ArrayHandle<vtkm::Vec<double, 3>, vtkm::cont::StorageTagBasic>>>(cellset, coords, scalarField, colorTable, camera, scalarRange);
      break;
    case 5:
      RenderCellsImplWithCoordType<vtkm::cont::ArrayHandleCast<vtkm::Vec<float, 3>, vtkm::cont::ArrayHandle<vtkm::Vec<double, 3>, vtkm::cont::StorageTagSOA>>>(cellset, coords, scalarField, colorTable, camera, scalarRange);
      break;
    case 6:
      RenderCellsImplWithCoordType<vtkm::cont::ArrayHandleCast<vtkm::Vec<float, 3>, vtkm::cont::ArrayHandle<vtkm::Vec<double, 3>, vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic, vtkm::cont::StorageTagBasic, vtkm::cont::StorageTagBasic>>>>(cellset, coords, scalarField, colorTable, camera, scalarRange);
      break;
    default:
      RenderCellsImplWithCoordType<vtkm::cont::CoordinateSystem::MultiplexerArrayType>(cellset, coords, scalarField, colorTable, camera, scalarRange);
      break;
  }
}
vtkm::rendering::Mapper* MapperVolume::NewCopy() const
{
  return new vtkm::rendering::MapperVolume(*this);
}

void MapperVolume::SetSampleDistance(const vtkm::Float32 sampleDistance)
{
  this->Internals->SampleDistance = sampleDistance;
}

void MapperVolume::SetCompositeBackground(const bool compositeBackground)
{
  this->Internals->CompositeBackground = compositeBackground;
}
}
} // namespace vtkm::rendering
