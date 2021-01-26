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
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;
  using V2 = vtkm::rendering::View2D;

  vtkm::cont::ColorTable colorTable("inferno");

  vtkm::rendering::testing::Render<M, C, V3>(
    vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_2.vtk"),
    "pointvar",
    colorTable,
    "rt_reg3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    vtkm::cont::testing::Testing::ReadVTKFile("rectilinear/RectilinearDataSet3D_0.vtk"),
    "pointvar",
    colorTable,
    "rt_rect3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_4.vtk"),
    "pointvar",
    colorTable,
    "rt_expl3D.pnm");

  vtkm::rendering::testing::Render<M, C, V2>(
    vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet2D_1.vtk"),
    "pointvar",
    colorTable,
    "uni2D.pnm");

  vtkm::rendering::testing::Render<M, C, V3>(
    vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_7.vtk"),
    "cellvar",
    colorTable,
    "spheres.pnm");
}

} //namespace

int UnitTestMapperRayTracer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
