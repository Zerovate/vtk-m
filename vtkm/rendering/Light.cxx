//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/rendering/Light.h>

namespace vtkm
{
namespace rendering
{
Light::Light() = default;

Light::~Light() = default;

Light::Light(const vtkm::Vec3f_32& position,
             const vtkm::Vec3f_32& color,
             const vtkm::Float32& intensity)
  : Position(position)
  , Color(color)
  , Intensity(intensity)
{
}

vtkm::Vec3f_32 Light::GetPosition() const
{
  return this->Position;
}

void Light::SetPosition(const vtkm::Vec3f_32& position)
{
  this->Position = position;
}

vtkm::Vec3f_32 Light::GetColor() const
{
  return this->Color;
}

void Light::SetColor(const vtkm::Vec3f_32 color)
{
  this->Color = color;
}

vtkm::Float32 Light::GetIntensity() const
{
  return this->Intensity;
}

void Light::SetIntensity(vtkm::Float32 intensity)
{
  this->Intensity = intensity;
}
}
}
