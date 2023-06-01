//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_PBRMaterial_h
#define vtk_m_rendering_PBRMaterial_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/cont/ColorTable.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/MaterialBase.h>
#include <vtkm/rendering/raytracing/Camera.h>

namespace vtkm
{
namespace rendering
{
class VTKM_RENDERING_EXPORT PBRMaterial : public vtkm::rendering::MaterialBase<PBRMaterial>
{
public:
  VTKM_CONT
  PBRMaterial();

  VTKM_CONT
  PBRMaterial(vtkm::Float32 metallic, vtkm::Float32 roughness);

  VTKM_CONT
  vtkm::Float32 GetMetallic() const;

  VTKM_CONT
  void SetMetallic(vtkm::Float32 metallic);

  VTKM_CONT
  vtkm::Float32 GetRoughness() const;

  VTKM_CONT
  void SetRoughness(vtkm::Float32 roughness);

  VTKM_CONT
  void Preprocess(const vtkm::rendering::raytracing::Camera& camera);

  template <typename Precision, typename LightCollection, typename CubeMap>
  VTKM_EXEC_CONT vtkm::Vec<Precision, 4> Evaluate(const vtkm::Vec<Precision, 4>& baseColor,
                                                  const vtkm::Vec<Precision, 3>& intersection,
                                                  const vtkm::Vec<Precision, 3>& normal,
                                                  const vtkm::Vec<Precision, 3>& view,
                                                  const LightCollection& lightCollection,
                                                  const CubeMap& cubeMap) const
  {
    using Vec3 = vtkm::Vec<Precision, 3>;
    using Vec4 = vtkm::Vec<Precision, 4>;

    Precision ao = 1.0f;
    Precision PI = static_cast<Precision>(vtkm::Pi());
    Vec3 F0(0.04, 0.04, 0.04);

    Vec4 color{ 0.0f };
    Vec3 N = normal;
    Vec3 V = view;
    F0 =
      vtkm::Lerp(F0, Vec3{ baseColor[0], baseColor[1], baseColor[2] }, Precision(this->Metallic));

    Vec3 Lo{ 0.0f };
    for (int i = 0; i < lightCollection.GetNumberOfLights(); ++i)
    {
      Vec3 L = lightCollection.GetL(i, intersection);
      Vec3 H = vtkm::Normal(V + L);

      vtkm::Vec3f_32 radiance = lightCollection.GetRadiance(i, intersection);

      Precision NDF = this->DistributionGGX(N, H, this->Roughness);
      Precision G = this->GeometrySmith(N, V, L, this->Roughness);
      Precision F_Vec_Par = vtkm::Clamp(vtkm::dot(H, V), Precision(0.0f), Precision(1.0f));
      Vec3 F(0.0f);
      this->FresnelSchlick(F_Vec_Par, F0, F);

      Vec3 kS = F;
      Vec3 kD = Vec3{ 1.0f, 1.0f, 1.0f } - kS;
      kD = kD * (1.0f - this->Metallic);
      Vec3 numerator = NDF * G * F;
      Precision denominator = 4.0f * vtkm::Max(vtkm::dot(N, V), Precision(0.0f)) *
          vtkm::Max(vtkm::dot(N, L), Precision(0.0f)) +
        0.0001f;
      Vec3 specular = numerator / denominator;

      Precision NdotL = vtkm::Max(vtkm::dot(N, L), Precision(0.0f));
      Lo += (kD * baseColor / PI + specular) * radiance * NdotL;
    }

    Vec3 F(0.0f);
    Precision F_Vec_Par = vtkm::Max(vtkm::Dot(N, V), Precision(0.0f));
    this->FresnelSchlick(F_Vec_Par, F0, F);
    Vec3 kS = F;
    Vec3 kD = Vec3{ 1.0f, 1.0f, 1.0f } - kS;
    kD = kD * (1.0f - this->Metallic);
    Vec3 R = vtkm::Normal(2.0f * vtkm::Dot(N, V) * N - V);
    Vec3 ambient;
    if (cubeMap.GetLoaded())
    {
      Vec3 irradiance = cubeMap.GetColor(R);
      Vec3 diffuse = irradiance * baseColor;
      ambient = kD * diffuse * ao;
    }
    else
    {
      ambient = Vec3(0.03f) * baseColor * ao;
    }

    color[0] = Lo[0] + ambient[0];
    color[1] = Lo[1] + ambient[1];
    color[2] = Lo[2] + ambient[2];
    color[3] = 1.0f;

    for (int i = 0; i < 3; i++)
    {
      color[i] = color[i] / (color[i] + 1.0f);
      color[i] = vtkm::Pow(color[i], Precision(1.0f / 2.2f));
    }

    return color;
  }

  template <typename Precision>
  float DistributionGGX(vtkm::Vec<Precision, 3>& N,
                        vtkm::Vec<Precision, 3>& H,
                        float roughness) const
  {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = vtkm::Max(vtkm::dot(N, H), Precision(0.0f));
    float NdotH2 = NdotH * NdotH;
    float PI = 3.14159265359f;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / denom;
  }

  float GeometrySchlickGGX(float NdotV, float roughness) const
  {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
  }

  template <typename Precision>
  float GeometrySmith(vtkm::Vec<Precision, 3>& N,
                      vtkm::Vec<Precision, 3>& V,
                      vtkm::Vec<Precision, 3>& L,
                      float roughness) const
  {
    float NdotV = vtkm::Max(vtkm::dot(N, V), Precision(0.0f));
    float NdotL = vtkm::Max(vtkm::dot(N, L), Precision(0.0f));
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
  }


  template <typename Precision>
  void FresnelSchlick(float cosTheta,
                      vtkm::Vec<Precision, 3>& F0,
                      vtkm::Vec<Precision, 3>& result) const
  {
    //clamp
    float a = 1.0f - cosTheta;
    if (a > 1.0f)
    {
      a = 1.0f;
    }
    else if (a < 0.0f)
    {
      a = 0.0f;
    }

    result = F0 + (1.0f - F0) * pow(a, 5.0f);
  }

private:
  vtkm::Float32 Metallic;
  vtkm::Float32 Roughness;
};
} // namespace rendering
} //namespace vtkm

#endif //vtk_m_rendering_PBRMaterial_h
