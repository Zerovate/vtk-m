//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/PhongMaterial.h>

namespace vtkm
{
namespace rendering
{
PhongMaterial::PhongMaterial() {}

void PhongMaterial::Preprocess(const vtkm::rendering::raytracing::Camera& camera)
{
  vtkm::Vec3f_32 scale(2, 2, 2);
  this->LightPosition = camera.GetPosition() + scale * camera.GetUp();

  this->LightAmbient = vtkm::Vec3f_32{ 0.5f, 0.5f, 0.5f };
  this->LightDiffuse = vtkm::Vec3f_32{ 0.7f, 0.7f, 0.7f };
  this->LightSpecular = vtkm::Vec3f_32{ 0.7f, 0.7f, 0.7f };
  this->SpecularExponent = 20.0f;
}
}
}
