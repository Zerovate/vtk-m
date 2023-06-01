//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MaterialGeneral_h
#define vtk_m_rendering_MaterialGeneral_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Types.h>
#include <vtkm/exec/Variant.h>
#include <vtkm/rendering/MaterialBase.h>
#include <vtkm/rendering/PBRMaterial.h>
#include <vtkm/rendering/PhongMaterial.h>

namespace vtkm
{
namespace rendering
{
namespace detail
{
template <typename Precision>
struct MaterialEvaluateFunctor
{
  template <typename MaterialType, typename LightCollection, typename CubeMap>
  VTKM_EXEC_CONT vtkm::Vec<Precision, 4> operator()(const MaterialType& material,
                                                    const vtkm::Vec<Precision, 4>& baseColor,
                                                    const vtkm::Vec<Precision, 3>& intersection,
                                                    const vtkm::Vec<Precision, 3>& normal,
                                                    const vtkm::Vec<Precision, 3>& view,
                                                    const LightCollection& lightCollection,
                                                    const CubeMap& cubeMap) const
  {
    return material.template Evaluate<Precision>(
      baseColor, intersection, normal, view, lightCollection, cubeMap);
  }
};

struct MaterialPreprocessFunctor
{
  template <typename MaterialType>
  VTKM_CONT void operator()(MaterialType& material,
                            const vtkm::rendering::raytracing::Camera& camera) const
  {
    return material.Preprocess(camera);
  }
};
}

template <typename... MaterialTypes>
struct MaterialMultiplexer
  : public vtkm::rendering::MaterialBase<MaterialMultiplexer<MaterialTypes...>>
{
  MaterialMultiplexer() = default;

  template <typename MaterialType>
  VTKM_EXEC_CONT MaterialMultiplexer(const vtkm::rendering::MaterialBase<MaterialType>& material)
    : Variant(reinterpret_cast<const MaterialType&>(material))
  {
  }

  VTKM_CONT void Preprocess(const vtkm::rendering::raytracing::Camera& camera)
  {
    return this->Variant.CastAndCall(detail::MaterialPreprocessFunctor{}, camera);
  }

  template <typename Precision, typename LightCollection, typename CubeMap>
  VTKM_EXEC_CONT vtkm::Vec<Precision, 4> Evaluate(const vtkm::Vec<Precision, 4>& baseColor,
                                                  const vtkm::Vec<Precision, 3>& intersection,
                                                  const vtkm::Vec<Precision, 3>& normal,
                                                  const vtkm::Vec<Precision, 3>& view,
                                                  const LightCollection& lightCollection,
                                                  const CubeMap& cubeMap) const
  {
    return this->Variant.CastAndCall(detail::MaterialEvaluateFunctor<Precision>{},
                                     baseColor,
                                     intersection,
                                     normal,
                                     view,
                                     lightCollection,
                                     cubeMap);
  }

  vtkm::exec::Variant<MaterialTypes...> Variant;
};

class MaterialGeneral
  : public vtkm::rendering::MaterialMultiplexer<vtkm::rendering::PBRMaterial,
                                                vtkm::rendering::PhongMaterial>
{
  using Superclass = vtkm::rendering::MaterialMultiplexer<vtkm::rendering::PBRMaterial,
                                                          vtkm::rendering::PhongMaterial>;

public:
  MaterialGeneral() = default;

  MaterialGeneral(const vtkm::rendering::PBRMaterial& material)
    : Superclass(material)
  {
  }

  MaterialGeneral(const vtkm::rendering::PhongMaterial& material)
    : Superclass(material)
  {
  }
};
} // namespace rendering
} //namespace vtkm

#endif //vtk_m_rendering_MaterialGeneral_h
