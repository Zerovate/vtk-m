//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/PBRMaterial.h>

namespace vtkm
{
namespace rendering
{
namespace
{
const vtkm::Float32 DEFAULT_METALLIC = 0.8f;
const vtkm::Float32 DEFAULT_ROUGHNESS = 0.8f;
}

PBRMaterial::PBRMaterial()
  : PBRMaterial(DEFAULT_METALLIC, DEFAULT_ROUGHNESS)
{
}

PBRMaterial::PBRMaterial(vtkm::Float32 metallic, vtkm::Float32 roughness)
  : Metallic(metallic)
  , Roughness(roughness)
{
}

vtkm::Float32 PBRMaterial::GetMetallic() const
{
  return this->Metallic;
}

void PBRMaterial::SetMetallic(vtkm::Float32 metallic)
{
  this->Metallic = metallic;
}

vtkm::Float32 PBRMaterial::GetRoughness() const
{
  return this->Roughness;
}

void PBRMaterial::SetRoughness(vtkm::Float32 roughness)
{
  this->Roughness = roughness;
}

void PBRMaterial::Preprocess(const vtkm::rendering::raytracing::Camera& vtkmNotUsed(camera)) {}
}
}
