//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_Texture2D_h
#define vtk_m_rendering_Texture2D_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace rendering
{

enum class TextureFilterMode
{
  NearestNeighbour,
  Linear,
}; // enum TextureFilterMode

enum class TextureWrapMode
{
  Clamp,
  Repeat,
}; // enum TextureWrapMode

using RGBPixelFormat = vtkm::Vec3f_32;
using RGBAPixelFormat = vtkm::Vec4f_32;

template <typename InputFormat, typename OutputFormat>
VTKM_EXEC inline void ConvertPixel(const InputFormat& input, OutputFormat& output)
{
  vtkm::IdComponent numComponents =
    vtkm::Min(input.GetNumberOfComponents(), output.GetNumberOfComponents());
  for (vtkm::IdComponent i = 0; i < numComponents; ++i)
  {
    output[i] = input[i];
  }
}

template <vtkm::IdComponent NumComponents, typename Precision>
VTKM_EXEC inline void ConvertPixel(const vtkm::Vec<vtkm::UInt8, NumComponents>& input,
                                   vtkm::Vec<Precision, NumComponents>& output)
{
  for (vtkm::IdComponent i = 0; i < NumComponents; ++i)
  {
    output[i] = static_cast<Precision>(input[i]) / 255.0f;
  }
}

template <typename InputPixelFormat, typename OutputPixelFormat = InputPixelFormat>
class Texture2D : public vtkm::cont::ExecutionObjectBase
{
public:
  using DataHandle = typename vtkm::cont::ArrayHandle<InputPixelFormat>;
  using DataPortal = typename DataHandle::ReadPortalType;

  template <typename Device>
  class Texture2DSampler;

#define UV_BOUNDS_CHECK(u, v, NoneType)             \
  if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) \
  {                                                 \
    return NoneType();                              \
  }

  VTKM_CONT
  Texture2D()
    : Width(0)
    , Height(0)
    , FlipY(false)
  {
  }

  VTKM_CONT
  Texture2D(vtkm::Id width, vtkm::Id height, const DataHandle& data)
    : Width(width)
    , Height(height)
    , FilterMode(TextureFilterMode::Linear)
    , WrapMode(TextureWrapMode::Clamp)
    , FlipY(false)
  {
    VTKM_ASSERT(data.GetNumberOfValues() == (Width * Height));
    // We do not know the lifetime of the underlying data source of input `data`. Since it might
    // be from a shallow copy of the data source, so we make a deep copy of the input data. The
    // copy operation is very fast.
    this->Data.DeepCopyFrom(data);
  }

  VTKM_CONT
  bool IsValid() const { return Width > 0 && Height > 0; }

  VTKM_CONT
  TextureFilterMode GetFilterMode() const { return this->FilterMode; }

  VTKM_CONT
  void SetFilterMode(TextureFilterMode filterMode) { this->FilterMode = filterMode; }

  VTKM_CONT
  TextureWrapMode GetWrapMode() const { return this->WrapMode; }

  VTKM_CONT
  void SetWrapMode(TextureWrapMode wrapMode) { this->WrapMode = wrapMode; }

  VTKM_CONT
  bool GetFlipY() const { return this->FlipY; }

  VTKM_CONT
  void SetFlipY(bool flipY) { this->FlipY = flipY; }

  template <typename Device>
  VTKM_CONT Texture2DSampler<Device> PrepareForExecution(Device, vtkm::cont::Token& token) const
  {
    return Texture2DSampler<Device>(
      this->Width, this->Height, this->Data, this->FilterMode, this->WrapMode, this->FlipY, token);
  }

  template <typename Device>
  class Texture2DSampler
  {
  public:
    VTKM_CONT
    Texture2DSampler()
      : Width(0)
      , Height(0)
      , FlipY(false)
    {
    }

    VTKM_CONT
    Texture2DSampler(vtkm::Id width,
                     vtkm::Id height,
                     const DataHandle& data,
                     TextureFilterMode filterMode,
                     TextureWrapMode wrapMode,
                     bool flipY,
                     vtkm::cont::Token& token)
      : Width(width)
      , Height(height)
      , Data(data.PrepareForInput(Device(), token))
      , FilterMode(filterMode)
      , WrapMode(wrapMode)
      , FlipY(flipY)
    {
    }

    VTKM_EXEC
    inline OutputPixelFormat GetColor(vtkm::Float32 u, vtkm::Float32 v) const
    {
      if (this->FlipY)
      {
        v = 1.0f - v;
      }
      UV_BOUNDS_CHECK(u, v, OutputPixelFormat);
      switch (FilterMode)
      {
        case TextureFilterMode::NearestNeighbour:
          return GetNearestNeighbourFilteredColor(u, v);

        case TextureFilterMode::Linear:
          return GetLinearFilteredColor(u, v);

        default:
          return OutputPixelFormat();
      }
    }

  private:
    VTKM_EXEC
    inline OutputPixelFormat GetNearestNeighbourFilteredColor(vtkm::Float32 u,
                                                              vtkm::Float32 v) const
    {
      vtkm::Id x = static_cast<vtkm::Id>(vtkm::Round(u * static_cast<vtkm::Float32>(Width - 1)));
      vtkm::Id y = static_cast<vtkm::Id>(vtkm::Round(v * static_cast<vtkm::Float32>(Height - 1)));
      return GetColorAtCoords(x, y);
    }

    VTKM_EXEC
    inline OutputPixelFormat GetLinearFilteredColor(vtkm::Float32 u, vtkm::Float32 v) const
    {
      u = u * static_cast<vtkm::Float32>(this->Width - 1);
      v = v * static_cast<vtkm::Float32>(this->Height - 1);
      vtkm::Id x = static_cast<vtkm::Id>(vtkm::Floor(u));
      vtkm::Id y = static_cast<vtkm::Id>(vtkm::Floor(v));
      vtkm::Float32 uRatio = u - static_cast<vtkm::Float32>(x);
      vtkm::Float32 vRatio = v - static_cast<vtkm::Float32>(y);
      vtkm::Float32 uOpposite = 1.0f - uRatio;
      vtkm::Float32 vOpposite = 1.0f - vRatio;
      vtkm::Id xn, yn;
      GetNextCoords(x, y, xn, yn);
      OutputPixelFormat c1 = GetColorAtCoords(x, y);
      OutputPixelFormat c2 = GetColorAtCoords(xn, y);
      OutputPixelFormat c3 = GetColorAtCoords(x, yn);
      OutputPixelFormat c4 = GetColorAtCoords(xn, yn);
      return (c1 * uOpposite + c2 * uRatio) * vOpposite + (c3 * uOpposite + c4 * uRatio) * vRatio;
    }

    VTKM_EXEC
    inline OutputPixelFormat GetColorAtCoords(vtkm::Id x, vtkm::Id y) const
    {
      vtkm::Id idx = (y * Width + x);
      OutputPixelFormat color;
      ConvertPixel(Data.Get(idx), color);
      return color;
    }

    VTKM_EXEC
    inline void GetNextCoords(vtkm::Id x, vtkm::Id y, vtkm::Id& xn, vtkm::Id& yn) const
    {
      switch (WrapMode)
      {
        case TextureWrapMode::Clamp:
          xn = (x + 1) < Width ? (x + 1) : x;
          yn = (y + 1) < Height ? (y + 1) : y;
          break;
        case TextureWrapMode::Repeat:
        default:
          xn = (x + 1) % Width;
          yn = (y + 1) % Height;
          break;
      }
    }

    vtkm::Id Width;
    vtkm::Id Height;
    DataPortal Data;
    TextureFilterMode FilterMode;
    TextureWrapMode WrapMode;
    bool FlipY;
  };

private:
  vtkm::Id Width;
  vtkm::Id Height;
  DataHandle Data;
  TextureFilterMode FilterMode;
  TextureWrapMode WrapMode;
  bool FlipY;
}; // class Texture2D
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_Texture2D_h
