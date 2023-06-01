//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_PointLight_h
#define vtk_m_rendering_PointLight_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Light.h>

namespace vtkm
{
namespace rendering
{
struct VTKM_RENDERING_EXPORT PointLight : public vtkm::rendering::Light
{
public:
  template <typename Device>
  class PointLightSampler;

  VTKM_CONT
  PointLight();

  VTKM_CONT
  PointLight(const vtkm::Vec3f_32& position,
             const vtkm::Vec3f_32& color,
             const vtkm::Float32& intensity);

  VTKM_CONT
  virtual LightType GetType() const override;

  VTKM_CONT
  virtual std::shared_ptr<Light> NewCopy() const override;
};
}
}

#endif // vtk_m_rendering_Light_h
