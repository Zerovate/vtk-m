//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperPoint.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  using M = vtkm::rendering::MapperPoint;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  vtkm::cont::ColorTable colorTable("inferno");

  M mapper;
  std::cout << "Testing uniform delta raduis\n";
  mapper.SetRadiusDelta(4.0f);
  vtkm::rendering::testing::Render<M, C, V3>(
    mapper,
    vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_1.vtk"),
    "pointvar",
    colorTable,
    "points_vr_reg3D.pnm");

  // restore defaults
  mapper.SetRadiusDelta(0.5f);
  mapper.UseVariableRadius(false);

  mapper.SetRadius(0.2f);
  vtkm::rendering::testing::Render<M, C, V3>(
    mapper,
    vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_1.vtk"),
    "pointvar",
    colorTable,
    "points_reg3D.pnm");

  mapper.UseCells();
  mapper.SetRadius(1.f);
  vtkm::rendering::testing::Render<M, C, V3>(
    mapper,
    vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_7.vtk"),
    "cellvar",
    colorTable,
    "spheres.pnm");
}

} //namespace

int UnitTestMapperPoints(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
