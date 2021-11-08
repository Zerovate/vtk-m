//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/cont/ArrayCopy.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

void RayOperations::MapCanvasToRays(Ray<vtkm::Float32>& rays,
                                    const vtkm::rendering::Camera& camera,
                                    const vtkm::rendering::CanvasRayTracer& canvas)
{
  vtkm::Id width = canvas.GetWidth();
  vtkm::Id height = canvas.GetHeight();
  std::cout << "        MatrixMultiply" << std::endl;
  vtkm::Matrix<vtkm::Float32, 4, 4> projview =
    vtkm::MatrixMultiply(camera.CreateProjectionMatrix(width, height), camera.CreateViewMatrix());
  bool valid;
  std::cout << "        MatrixInverse" << std::endl;
  vtkm::Matrix<vtkm::Float32, 4, 4> inverse = vtkm::MatrixInverse(projview, valid);
  std::cout << "        valid: " << valid << std::endl;
  (void)valid; // this can be a false negative for really tiny spatial domains.
  vtkm::cont::printSummary_ArrayHandle(rays.PixelIdx, std::cout);
  vtkm::cont::printSummary_ArrayHandle(rays.MaxDistance, std::cout);
  vtkm::cont::printSummary_ArrayHandle(rays.Origin, std::cout);
  vtkm::cont::printSummary_ArrayHandle(canvas.GetDepthBuffer(), std::cout);
  std::cout << "        Copy Origin" << std::endl;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_32> originCopy;
  vtkm::cont::ArrayCopy(rays.Origin, originCopy);
  std::cout << "        RayMapCanvas" << std::endl;
  vtkm::worklet::DispatcherMapField<detail::RayMapCanvas>(
    detail::RayMapCanvas(inverse, width, height, camera.GetPosition()))
    .Invoke(rays.PixelIdx, rays.MaxDistance, originCopy, canvas.GetDepthBuffer());
  std::cout << "        MapCanvasToRays complete" << std::endl;
}
}
}
} // vtkm::rendering::raytacing
