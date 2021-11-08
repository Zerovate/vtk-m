//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperQuad.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/Cylinderizer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/QuadExtractor.h>
#include <vtkm/rendering/raytracing/QuadIntersector.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracer.h>

namespace vtkm
{
namespace rendering
{

struct MapperQuad::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::rendering::raytracing::RayTracer Tracer;
  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::rendering::raytracing::Ray<vtkm::Float32> Rays;
  bool CompositeBackground;
  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
  {
  }
};

MapperQuad::MapperQuad()
  : Internals(new InternalsType)
{
}

MapperQuad::~MapperQuad() {}

void MapperQuad::SetCanvas(vtkm::rendering::Canvas* canvas)
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

vtkm::rendering::Canvas* MapperQuad::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperQuad::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                             const vtkm::cont::CoordinateSystem& coords,
                             const vtkm::cont::Field& scalarField,
                             const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                             const vtkm::rendering::Camera& camera,
                             const vtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_ray_tracer");
  vtkm::cont::Timer tot_timer;
  tot_timer.Start();
  vtkm::cont::Timer timer;


  //
  // Add supported shapes
  //
  std::cout << "      extracting quads" << std::endl;
  vtkm::Bounds shapeBounds;
  raytracing::QuadExtractor quadExtractor;
  quadExtractor.ExtractCells(cellset);
  if (quadExtractor.GetNumberOfQuads() > 0)
  {
    std::cout << "      adding quads" << std::endl;
    auto quadIntersector = std::make_shared<raytracing::QuadIntersector>();
    quadIntersector->SetData(coords, quadExtractor.GetQuadIds());
    this->Internals->Tracer.AddShapeIntersector(quadIntersector);
    shapeBounds.Include(quadIntersector->GetShapeBounds());
  }

  //
  // Create rays
  //
  std::cout << "      creating rays" << std::endl;
  vtkm::Int32 width = (vtkm::Int32)this->Internals->Canvas->GetWidth();
  vtkm::Int32 height = (vtkm::Int32)this->Internals->Canvas->GetHeight();

  this->Internals->RayCamera.SetParameters(camera, width, height);

  std::cout << "      RayCamera.CreateRays" << std::endl;
  this->Internals->RayCamera.CreateRays(this->Internals->Rays, shapeBounds);
  std::cout << "      Rays.Buffers.at(0).InitConst(0.f)" << std::endl;
  std::cout << "      " << this->Internals->Rays.Buffers.size() << std::endl;
  this->Internals->Rays.Buffers.at(0).InitConst(0.f);
  std::cout << "      RayOperations::MapCanvasToRays" << std::endl;
  raytracing::RayOperations::MapCanvasToRays(
    this->Internals->Rays, camera, *this->Internals->Canvas);

  std::cout << "      rendering rays" << std::endl;
  this->Internals->Tracer.GetCamera() = this->Internals->RayCamera;
  this->Internals->Tracer.SetField(scalarField, scalarRange);
  this->Internals->Tracer.SetColorMap(this->ColorMap);
  this->Internals->Tracer.Render(this->Internals->Rays);

  timer.Start();
  std::cout << "      write rays" << std::endl;
  this->Internals->Canvas->WriteToCanvas(
    this->Internals->Rays, this->Internals->Rays.Buffers.at(0).Buffer, camera);

  if (this->Internals->CompositeBackground)
  {
    std::cout << "      composite background" << std::endl;
    this->Internals->Canvas->BlendBackground();
  }

  std::cout << "      log results" << std::endl;
  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
  std::cout << "      done" << std::endl;
}

void MapperQuad::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

vtkm::rendering::Mapper* MapperQuad::NewCopy() const
{
  return new vtkm::rendering::MapperQuad(*this);
}
}
} // vtkm::rendering
