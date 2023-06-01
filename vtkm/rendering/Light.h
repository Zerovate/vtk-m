//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_Light_h
#define vtk_m_rendering_Light_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Types.h>

#include <memory>

namespace vtkm
{
namespace rendering
{
enum class LightType
{
  PointLight
};

struct VTKM_RENDERING_EXPORT Light
{
public:
  virtual ~Light();

  VTKM_CONT
  vtkm::Vec3f_32 GetPosition() const;

  VTKM_CONT
  void SetPosition(const vtkm::Vec3f_32& position);

  VTKM_CONT
  vtkm::Vec3f_32 GetColor() const;

  VTKM_CONT
  void SetColor(const vtkm::Vec3f_32 color);

  VTKM_CONT
  vtkm::Float32 GetIntensity() const;

  VTKM_CONT
  void SetIntensity(vtkm::Float32 intensity);

  VTKM_CONT
  virtual LightType GetType() const = 0;

  VTKM_CONT
  virtual std::shared_ptr<Light> NewCopy() const = 0;

protected:
  Light();

  Light(const vtkm::Vec3f_32& position,
        const vtkm::Vec3f_32& color,
        const vtkm::Float32& intensity);

  vtkm::Vec3f_32 Position;
  vtkm::Vec3f_32 Color;
  vtkm::Float32 Intensity;
};
}
}

#endif // vtk_m_rendering_Light_h
