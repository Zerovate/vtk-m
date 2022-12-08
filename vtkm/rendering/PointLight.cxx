//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/PointLight.h>

namespace vtkm
{
namespace rendering
{
PointLight::PointLight()
  : PointLight({ 0.0f }, { 0.0f }, 0.0f)
{
}

PointLight::PointLight(const vtkm::Vec3f_32& position,
                       const vtkm::Vec3f_32& color,
                       const vtkm::Float32& intensity)
  : Light(position, color, intensity)
{
}

LightType PointLight::GetType() const
{
  return LightType::PointLight;
}

std::shared_ptr<Light> PointLight::NewCopy() const
{
  return std::shared_ptr<PointLight>(new PointLight(*this));
}
}
}
