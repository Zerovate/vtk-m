//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_CubeMap_h
#define vtk_m_rendering_CubeMap_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/rendering/Texture2D.h>

namespace vtkm
{
namespace rendering
{
namespace
{
template <typename Precision>
VTKM_EXEC inline Precision ApproxEquals(Precision x, Precision y, Precision eps = 1e-5f)
{
  return vtkm::Abs(x - y) <= eps;
}
} // namespace

class CubeMap : public vtkm::cont::ExecutionObjectBase
{
public:
  using InputPixelType = vtkm::Vec4f_32;
  using OutputPixelType = vtkm::Vec3f_32;
  using FaceTexture = vtkm::rendering::Texture2D<InputPixelType, OutputPixelType>;

  VTKM_CONT
  CubeMap()
    : Loaded(false)
  {
  }

  VTKM_CONT
  CubeMap(const FaceTexture& posX,
          const FaceTexture& negX,
          const FaceTexture& posY,
          const FaceTexture& negY,
          const FaceTexture& posZ,
          const FaceTexture& negZ)
    : Faces(posX, negX, posY, negY, posZ, negZ)
    , Loaded(true)
  {
  }

  template <typename Device>
  class CubeMapSampler
  {
  public:
    using Texture2DSampler = typename FaceTexture::Texture2DSampler<Device>;

    VTKM_CONT
    CubeMapSampler()
      : Loaded(false)
    {
    }

    VTKM_CONT
    CubeMapSampler(const vtkm::Vec<FaceTexture, 6>& faces, vtkm::cont::Token& token)
      : Loaded(true)
    {
      for (vtkm::IdComponent i = 0; i < 6; ++i)
      {
        this->FaceSamplers[i] = faces[i].PrepareForExecution(Device(), token);
      }
    }

    VTKM_EXEC
    inline OutputPixelType GetColor(const vtkm::Vec3f_32& direction) const
    {
      return this->GetColorImpl(direction);
    }

    VTKM_EXEC
    inline OutputPixelType GetColor(const vtkm::Vec3f_64& direction) const
    {
      vtkm::Vec3f_32 direction32{ static_cast<vtkm::Float32>(direction[0]),
                                  static_cast<vtkm::Float32>(direction[1]),
                                  static_cast<vtkm::Float32>(direction[2]) };
      return this->GetColorImpl(direction32);
    }

    VTKM_EXEC_CONT
    inline bool GetLoaded() const { return this->Loaded; }

  private:
    VTKM_EXEC
    inline OutputPixelType GetColorImpl(const vtkm::Vec3f_32& direction) const
    {
      vtkm::Float32 u, v;
      const vtkm::IdComponent faceIdx = this->GetFaceIndexInDirection(direction, u, v);
      return this->GetColor(faceIdx, u, v);
    }

    VTKM_EXEC
    inline vtkm::IdComponent GetFaceIndexInDirection(const vtkm::Vec3f_32& direction,
                                                     vtkm::Float32& u,
                                                     vtkm::Float32& v) const
    {
      vtkm::Float32 x = direction[0];
      vtkm::Float32 y = direction[1];
      vtkm::Float32 z = direction[2];
      vtkm::Float32 absX = vtkm::Abs(x);
      vtkm::Float32 absY = vtkm::Abs(y);
      vtkm::Float32 absZ = vtkm::Abs(z);

      bool isXPositive = x > 0.0f;
      bool isYPositive = y > 0.0f;
      bool isZPositive = z > 0.0f;

      vtkm::Float32 maxAxis = 0.0f;
      vtkm::Float32 uc = 0.0f;
      vtkm::Float32 vc = 0.0f;
      vtkm::IdComponent faceIdx = 0;

      // POSITIVE X
      if (isXPositive && absX >= absY && absX >= absZ)
      {
        maxAxis = absX;
        uc = -z;
        vc = y;
        faceIdx = 0;
      }
      // NEGATIVE X
      if (!isXPositive && absX >= absY && absX >= absZ)
      {
        maxAxis = absX;
        uc = z;
        vc = y;
        faceIdx = 1;
      }
      // POSITIVE Y
      if (isYPositive && absY >= absX && absY >= absZ)
      {
        maxAxis = absY;
        uc = x;
        vc = -z;
        faceIdx = 2;
      }
      // NEGATIVE Y
      if (!isYPositive && absY >= absX && absY >= absZ)
      {
        maxAxis = absY;
        uc = x;
        vc = z;
        faceIdx = 3;
      }
      // POSITIVE Z
      if (isZPositive && absZ >= absX && absZ >= absY)
      {
        maxAxis = absZ;
        uc = x;
        vc = y;
        faceIdx = 4;
      }
      // NEGATIVE Z
      if (!isZPositive && absZ >= absX && absZ >= absY)
      {
        maxAxis = absZ;
        uc = -x;
        vc = y;
        faceIdx = 5;
      }

      u = vtkm::Clamp(0.5f * (uc / maxAxis + 1.0f), 0.0f, 1.0f);
      v = vtkm::Clamp(0.5f * (vc / maxAxis + 1.0f), 0.0f, 1.0f);
      return faceIdx;
    }

    VTKM_EXEC
    inline OutputPixelType GetColor(const vtkm::IdComponent& faceIdx,
                                    const vtkm::Float32& u,
                                    const vtkm::Float32& v) const
    {
      const Texture2DSampler& faceSampler = this->FaceSamplers[faceIdx];
      return faceSampler.GetColor(u, v);
    }

    vtkm::Vec<Texture2DSampler, 6> FaceSamplers;
    bool Loaded;
  };

  template <typename Device>
  VTKM_CONT CubeMapSampler<Device> PrepareForExecution(Device, vtkm::cont::Token& token)
  {
    if (this->Loaded)
    {
      return CubeMapSampler<Device>(this->Faces, token);
    }
    else
    {
      return CubeMapSampler<Device>();
    }
  }

  VTKM_EXEC_CONT
  bool GetLoaded() const { return this->Loaded; }

private:
  vtkm::Vec<FaceTexture, 6> Faces;
  bool Loaded;
}; // class CubeMap
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_CubeMap_h
