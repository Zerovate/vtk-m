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
#include <vtkm/rendering/MapperCylinder.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  typedef vtkm::rendering::MapperCylinder M;
  typedef vtkm::rendering::CanvasRayTracer C;
  typedef vtkm::rendering::View3D V3;
  typedef vtkm::rendering::View2D V2;

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
    vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_8.vtk"),
    "cellvar",
    colorTable,
    "cylinder.pnm");

  //hexahedron, wedge, pyramid, tetrahedron
  vtkm::rendering::testing::Render<M, C, V3>(
    vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_5.vtk"),
    "cellvar",
    colorTable,
    "rt_hex3d.pnm");

  M mapper;

  mapper.SetRadius(0.1f);
  vtkm::rendering::testing::Render<M, C, V3>(
    mapper,
    vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_8.vtk"),
    "cellvar",
    colorTable,
    "cyl_static_radius.pnm");

  mapper.UseVariableRadius(true);
  mapper.SetRadiusDelta(2.0f);
  vtkm::rendering::testing::Render<M, C, V3>(
    mapper,
    vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_8.vtk"),
    "cellvar",
    colorTable,
    "cyl_var_radius.pnm");

  //test to make sure can reset
  mapper.UseVariableRadius(false);
  vtkm::rendering::testing::Render<M, C, V3>(
    vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_8.vtk"),
    "cellvar",
    colorTable,
    "cylinder2.pnm");
}

} //namespace

int UnitTestMapperCylinders(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
