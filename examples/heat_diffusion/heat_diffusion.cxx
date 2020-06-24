//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//This work was supported by Commissariat a l'Energie Atomique    * CEA, DAM, DIF, F-91297 Arpajon, France.

#ifdef ANIMATE
#include <GL/glew.h>
#include <GL/glut.h>

#include <vtkm/filter/WarpScalar.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#endif

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/filter/FilterDataSet.h>

#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/worklet/DispatcherMapField.h>


#include <iomanip>


#include "Diffusion_Filter.h"
#include "parameters.h"

#define HEIGHT 900
#define WIDTH 900


vtkm::cont::DataSet create_Data_Set(const Parameters& params)
{

  vtkm::Id2 dimensions(params.dimension, params.dimension);
  vtkm::cont::DataSet dataSet = vtkm::cont::DataSetBuilderUniform::Create(
    dimensions,
    vtkm::Vec2f{ -2.0f, -2.0f },
    vtkm::Vec2f{ 4.0f / (dimensions[0] - 1), 4.0f / (dimensions[1] - 1) });

  vtkm::cont::CoordinateSystem coords = dataSet.GetCoordinateSystem("coords");

  vtkm::cont::ArrayHandle<vtkm::Int8> boundary;
  vtkm::cont::ArrayHandle<vtkm::Float32> temperature;
  vtkm::cont::ArrayHandle<vtkm::Float32> diffuseCoeff;

  vtkm::cont::Invoker invoke;

  invoke(FillInitialCondition{ params }, coords, boundary, temperature, diffuseCoeff);

  dataSet.AddField(vtkm::cont::make_FieldPoint("boundary_condition", boundary));
  dataSet.AddField(vtkm::cont::make_FieldPoint("temperature", temperature));
  dataSet.AddField(vtkm::cont::make_FieldPoint("coeff_diffusion", diffuseCoeff));


  return dataSet;
}

#ifdef ANIMATE
void display_function(vtkm::cont::DataSet* global_ptr_data, vtkm::cont::DataSet newData, bool dim)
{
  vtkm::filter::Diffusion filter;

  newData = filter.Execute(*global_ptr_data, DiffusionPolicy());
  vtkm::rendering::CanvasRayTracer canvas(WIDTH, HEIGHT);
  vtkm::rendering::MapperRayTracer mapper;

  vtkm::rendering::Scene scene;

  if (!dim)
  {
    auto normal = vtkm::cont::make_ArrayHandleConstant(vtkm::Vec3f_32{ 0.f, 0.f, 1.f },
                                                       newData.GetNumberOfPoints());
    newData.AddField(vtkm::cont::make_FieldPoint("normal", normal));

    vtkm::filter::WarpScalar warp(0.05f);

    warp.SetUseCoordinateSystemAsField(true);
    warp.SetScalarFactorField("temperature");
    warp.SetFieldsToPass("temperature");
    vtkm::cont::DataSet warpped = warp.Execute(newData);

    vtkm::cont::ArrayHandle<vtkm::Vec3f> warpped_points;
    warpped.GetPointField("warpscalar").GetData().CopyTo(warpped_points);

    vtkm::rendering::Actor actor(warpped.GetCellSet(),
                                 vtkm::cont::CoordinateSystem("warp", warpped_points),
                                 warpped.GetPointField("temperature"),
                                 vtkm::cont::ColorTable("Cool to warm"));

    scene.AddActor(actor);
  }
  else
  {
    vtkm::rendering::Actor actor(newData.GetCellSet(),
                                 newData.GetCoordinateSystem(),
                                 newData.GetPointField("temperature"),
                                 vtkm::cont::ColorTable("Cool to Warm"));

    scene.AddActor(actor);
  }

  vtkm::rendering::View3D view(scene, mapper, canvas);
  view.Initialize();

  if (!dim)
  {
    view.GetCamera().SetLookAt(vtkm::Vec3f_32{ 0.f, 0.f, 3.0f });
    view.GetCamera().Azimuth(45.0);
    view.GetCamera().Elevation(40.0);
  }

  view.Paint();
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> color_buffer = view.GetCanvas().GetColorBuffer();
  void* colorArray = color_buffer.GetStorage().GetBasePointer();
  glDrawPixels(
    view.GetCanvas().GetWidth(), view.GetCanvas().GetHeight(), GL_RGBA, GL_FLOAT, colorArray);
  glutSwapBuffers();

  *global_ptr_data = newData;
}

vtkm::cont::DataSet* global_ptr_data = nullptr;
vtkm::cont::DataSet newData;
bool dim;

#endif

int main(int argc, char* argv[])
{

  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  vtkm::cont::DataSet data;

  Parameters params;
  read_params(argc, argv, params);


  if (params.create_matrix)
  {
    data = create_Data_Set(params);
    std::cout << "Matrix size: " << params.dimension << "x" << params.dimension << std::endl;
    std::cout << "Temperature outside: " << std::get<0>(params.temperature) << std::endl;
    std::cout << "Temperature inside: " << std::get<1>(params.temperature) << std::endl;
    std::cout << "Diffusion coefficient: " << params.diffuse_coeff << std::endl;
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(config.Device);
  }
  else
  {
    vtkm::io::VTKDataSetReader reader(params.filename);
    data = reader.ReadDataSet();
  }

#ifndef ANIMATE
  if (params.rendering_enable)
  {
    std::cout << "Animation is not available : performance is running " << std::endl;
    params.rendering_enable = false;
  }

#endif

  if (params.rendering_enable)
  {
#ifdef ANIMATE
    global_ptr_data = &data;
    std::vector<int> ite;
    ite.push_back(10);

    vtkm::cont::DataSetFieldAdd::AddPointField(*global_ptr_data, "iteration", ite);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("VTK-m Heat Diffusion");

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      std::cout << "glewInit failed\n";
    }
    dim = params.twoD;
    glutDisplayFunc([] { display_function(global_ptr_data, newData, dim); });

    glutIdleFunc([]() { glutPostRedisplay(); });

    glutMainLoop();
#endif
  }
  else
  {
    vtkm::filter::Diffusion filter;
    std::vector<int> ite;
    ite.push_back(params.iteration);

    vtkm::cont::DataSetFieldAdd::AddPointField(data, "iteration", ite);
    vtkm::Float32 end, start;
    vtkm::cont::Timer gTimer;
    gTimer.Start();
    std::cout << "Number of iteration: " << params.iteration << std::endl;

    start = static_cast<vtkm::Float32>(gTimer.GetElapsedTime());
    data = filter.Execute(data);
    end = static_cast<vtkm::Float32>(gTimer.GetElapsedTime());

    std::cout << std::endl << "Execution time = " << end - start << std::endl;
    double flop = (params.dimension * params.dimension * 8.f / (end - start)) * params.iteration;
    std::cout << "MFlop = " << flop / 10e6 << std::endl;
  }


  return 0;
}
