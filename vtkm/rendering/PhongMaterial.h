//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_PhongMaterial_h
#define vtk_m_rendering_PhongMaterial_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/MaterialBase.h>
#include <vtkm/rendering/raytracing/Camera.h>

namespace vtkm
{
namespace rendering
{
class VTKM_RENDERING_EXPORT PhongMaterial : public vtkm::rendering::MaterialBase<PhongMaterial>
{
public:
  VTKM_CONT
  PhongMaterial();

  VTKM_CONT
  void Preprocess(const vtkm::rendering::raytracing::Camera& camera);

  template <typename Precision, typename LightCollection, typename CubeMap>
  VTKM_EXEC_CONT vtkm::Vec<Precision, 4> Evaluate(
    const vtkm::Vec<Precision, 4>& baseColor,
    const vtkm::Vec<Precision, 3>& intersection,
    const vtkm::Vec<Precision, 3>& normal,
    const vtkm::Vec<Precision, 3>& view,
    const LightCollection& vtkmNotUsed(lightCollection),
    const CubeMap& cubeMap) const
  {
    using Vec3 = vtkm::Vec<Precision, 3>;
    using Vec4 = vtkm::Vec<Precision, 4>;

    Vec3 lightDir = this->LightPosition - intersection;
    vtkm::Normalize(lightDir);

    //Diffuse lighting
    Precision cosTheta = vtkm::dot(normal, lightDir);
    //clamp tp [0,1]
    const Precision zero = 0.f;
    const Precision one = 1.f;
    cosTheta = vtkm::Min(vtkm::Max(cosTheta, zero), one);
    //Specular lighting
    Vec3 reflect = 2.f * vtkm::dot(lightDir, normal) * normal - lightDir;
    vtkm::Normalize(reflect);
    Precision cosPhi = vtkm::dot(reflect, view);
    Precision specularConstant =
      vtkm::Pow(vtkm::Max(cosPhi, zero), static_cast<Precision>(this->SpecularExponent));

    Vec4 color = baseColor;
    Vec3 ambient = this->LightAmbient;
    if (cubeMap.GetLoaded())
    {
      Vec3 cubeMapSample = cubeMap.GetColor(reflect);
      ambient[0] = cubeMapSample[0] * baseColor[0];
      ambient[1] = cubeMapSample[1] * baseColor[1];
      ambient[2] = cubeMapSample[2] * baseColor[2];
    }

    color[0] *= vtkm::Min(ambient[0] + this->LightDiffuse[0] * cosTheta +
                            this->LightSpecular[0] * specularConstant,
                          one);
    color[1] *= vtkm::Min(ambient[1] + this->LightDiffuse[1] * cosTheta +
                            this->LightSpecular[1] * specularConstant,
                          one);
    color[2] *= vtkm::Min(ambient[2] + this->LightDiffuse[2] * cosTheta +
                            this->LightSpecular[2] * specularConstant,
                          one);
    return color;
  }

private:
  vtkm::Vec3f_32 LightPosition;
  vtkm::Vec3f_32 LightAmbient;
  vtkm::Vec3f_32 LightDiffuse;
  vtkm::Vec3f_32 LightSpecular;
  vtkm::Float32 SpecularExponent;
};
} // namespace rendering
} //namespace vtkm

#endif //vtk_m_rendering_PhongMaterial_h
