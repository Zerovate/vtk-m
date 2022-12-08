//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/LightCollection.h>

namespace vtkm
{
namespace rendering
{
void LightCollection::AddLight(std::shared_ptr<vtkm::rendering::Light> light)
{
  this->Lights.push_back(light);
}

vtkm::IdComponent LightCollection::GetNumberOfLights() const
{
  return static_cast<vtkm::IdComponent>(this->Lights.size());
}

VTKM_CONT
std::shared_ptr<vtkm::rendering::Light> LightCollection::GetLight(
  vtkm::IdComponent lightIndex) const
{
  return this->Lights[lightIndex];
}

VTKM_CONT
void LightCollection::Clear()
{
  this->Lights.clear();
}
}
}
