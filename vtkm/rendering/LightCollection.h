//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_LightCollection_h
#define vtk_m_rendering_LightCollection_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/VecVariable.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/rendering/Light.h>
#include <vtkm/rendering/PointLight.h>

#include <memory>
#include <vector>

namespace vtkm
{
namespace rendering
{
namespace
{
inline vtkm::Vec3f_32 ToVec3f_32(const vtkm::Vec3f_64& v)
{
  vtkm::Vec3f_32 v_32;
  for (vtkm::IdComponent i = 0; i < 3; ++i)
  {
    v_32[i] = static_cast<vtkm::Float32>(v[i]);
  }

  return v_32;
}
} // namespace

struct VTKM_RENDERING_EXPORT LightCollection : public vtkm::cont::ExecutionObjectBase
{
public:
  const static vtkm::IdComponent MAX_NUM_LIGHTS = 10;

  template <typename Device>
  class LightCollectionExecObject;

  VTKM_CONT
  void AddLight(std::shared_ptr<vtkm::rendering::Light> light);

  VTKM_CONT
  vtkm::IdComponent GetNumberOfLights() const;

  VTKM_CONT
  std::shared_ptr<vtkm::rendering::Light> GetLight(vtkm::IdComponent lightIndex) const;

  VTKM_CONT
  void Clear();

  template <typename Device>
  VTKM_CONT LightCollectionExecObject<Device> PrepareForExecution(Device, vtkm::cont::Token& token)
  {
    return LightCollectionExecObject<Device>(this->Lights, Device(), token);
  }

  template <typename Device>
  struct LightCollectionExecObject
  {
    struct PointLightData
    {
      vtkm::rendering::LightType Type;
      vtkm::Vec3f_32 Position;
      vtkm::Vec3f_32 Color;
      vtkm::Float32 Intensity;
    };

    using LightSamplerVec = vtkm::VecVariable<PointLightData, MAX_NUM_LIGHTS>;

    VTKM_CONT
    LightCollectionExecObject(const std::vector<std::shared_ptr<vtkm::rendering::Light>>& lights,
                              Device,
                              vtkm::cont::Token& vtkmNotUsed(token))
    {
      for (std::size_t i = 0; i < lights.size() && i < MAX_NUM_LIGHTS; ++i)
      {
        std::shared_ptr<vtkm::rendering::Light> light = lights[i];
        if (light->GetType() == vtkm::rendering::LightType::PointLight)
        {
          PointLightData data;
          data.Type = light->GetType();
          data.Position = light->GetPosition();
          data.Color = light->GetColor();
          data.Intensity = light->GetIntensity();
          LightSamplers.Append(data);
        }
      }
    }

    VTKM_EXEC vtkm::IdComponent GetNumberOfLights() const
    {
      return this->LightSamplers.GetNumberOfComponents();
    }

    VTKM_EXEC vtkm::Vec3f_32 GetL(vtkm::IdComponent lightIndex, const vtkm::Vec3f_32& at) const
    {
      return this->GetLImpl(lightIndex, at);
    }

    VTKM_EXEC vtkm::Vec3f_32 GetL(vtkm::IdComponent lightIndex, const vtkm::Vec3f_64& at) const
    {
      return this->GetLImpl(lightIndex, ToVec3f_32(at));
    }

    VTKM_EXEC vtkm::Vec3f_32 GetRadiance(vtkm::IdComponent lightIndex,
                                         const vtkm::Vec3f_32& at) const
    {
      return this->GetRadianceImpl(lightIndex, at);
    }

    VTKM_EXEC vtkm::Vec3f_32 GetRadiance(vtkm::IdComponent lightIndex,
                                         const vtkm::Vec3f_64& at) const
    {
      return this->GetRadianceImpl(lightIndex, ToVec3f_32(at));
    }

  protected:
    VTKM_EXEC vtkm::Vec3f_32 GetLImpl(vtkm::IdComponent lightIndex, const vtkm::Vec3f_32& at) const
    {
      const PointLightData& data = this->LightSamplers[lightIndex];
      if (data.Type == vtkm::rendering::LightType::PointLight)
      {
        return vtkm::Normal(data.Position - at);
      }

      return vtkm::Vec3f_32{ 0.0f };
    }

    VTKM_EXEC vtkm::Vec3f_32 GetRadianceImpl(vtkm::IdComponent lightIndex,
                                             const vtkm::Vec3f_32& at) const
    {
      const PointLightData& data = this->LightSamplers[lightIndex];
      if (data.Type == vtkm::rendering::LightType::PointLight)
      {
        vtkm::Float32 distanceSquared = vtkm::MagnitudeSquared(data.Position - at);
        return data.Color * data.Intensity * (1.0f / distanceSquared);
      }

      return 0.0f;
    }

    LightSamplerVec LightSamplers;
  };

protected:
  std::vector<std::shared_ptr<vtkm::rendering::Light>> Lights;
};
}
}

#endif // vtk_m_rendering_LightCollection_h
