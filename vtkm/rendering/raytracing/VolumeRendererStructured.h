//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_VolumeRendererStructured_h
#define vtk_m_rendering_raytracing_VolumeRendererStructured_h

#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

template <typename CoordType = vtkm::cont::CoordinateSystem::MultiplexerArrayType>
class VTKM_RENDERING_EXPORT VolumeRendererStructured
{
public:
  VTKM_CONT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap);

  VTKM_CONT
  void SetData(const vtkm::cont::CoordinateSystem& coords,
               const vtkm::cont::Field& scalarField,
               const vtkm::cont::CellSetStructured<3>& cellset,
               const vtkm::Range& scalarRange);

  VTKM_CONT
  void SetData(const vtkm::cont::CoordinateSystem& coords,
                              const vtkm::cont::Field& scalarField,
                              const vtkm::cont::CellSetSingleType<>& cellset,
                              const vtkm::Range& scalarRange,
                              const vtkm::FloatDefault L1 = -1.f,
                              const vtkm::FloatDefault L2 = -1.f);

  VTKM_CONT
  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays);
  //VTKM_CONT
  ///void Render(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays);


  VTKM_CONT
  void SetSampleDistance(const vtkm::Float32& distance);

protected:
  template <typename Precision, typename Device>
  VTKM_CONT void RenderOnDevice(vtkm::rendering::raytracing::Ray<Precision>& rays, Device);

  bool IsSceneDirty = false;
  bool IsStructuredDataSet = false;
  bool IsUniformDataSet = true;
  vtkm::Bounds SpatialExtent;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkm::cont::CellSetStructured<3> Cellset;
  vtkm::cont::CellSetSingleType<> CellsetUnstruct;
  const vtkm::cont::Field* ScalarField;
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> ColorMap;
  vtkm::Float32 SampleDistance = -1.f;
  vtkm::Range ScalarRange;
  vtkm::cont::CellLocatorTwoLevel<CoordType> CellLocator;
};
}
}
} //namespace vtkm::rendering::raytracing
#endif
