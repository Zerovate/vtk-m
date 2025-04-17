//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_exec_CubicHermiteSpline_h
#define vtk_m_exec_CubicHermiteSpline_h

#include <vtkm/ErrorCode.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace exec
{

class VTKM_ALWAYS_EXPORT CubicHermiteSpline
{
private:
  using DataArrayHandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using KnotsArrayHandleType = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using TangentArrayHandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using DataPortalType = typename DataArrayHandleType::ReadPortalType;
  using KnotsPortalType = typename KnotsArrayHandleType::ReadPortalType;
  using TangentPortalType = typename TangentArrayHandleType::ReadPortalType;

public:
  VTKM_CONT CubicHermiteSpline(const vtkm::cont::ArrayHandle<vtkm::Vec3f>& data,
                               const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& knots,
                               const vtkm::cont::ArrayHandle<vtkm::Vec3f>& tangents,
                               vtkm::cont::DeviceAdapterId device,
                               vtkm::cont::Token& token)
  {
    this->Data = data.PrepareForInput(device, token);
    this->Knots = knots.PrepareForInput(device, token);
    this->Tangents = tangents.PrepareForInput(device, token);
  }

  VTKM_EXEC
  vtkm::ErrorCode Evaluate(const vtkm::FloatDefault& tVal, vtkm::Vec3f& val) const
  {
    vtkm::Id idx = this->FindInterval(tVal);
    if (idx < 0)
    {
      idx = this->FindInterval(tVal);
      return vtkm::ErrorCode::ValueOutOfRange;
    }

    auto m0 = this->Tangents.Get(idx);
    auto m1 = this->Tangents.Get(idx + 1);

    vtkm::FloatDefault t0 = this->Knots.Get(idx);
    vtkm::FloatDefault t1 = this->Knots.Get(idx + 1);
    vtkm::FloatDefault t = (tVal - t0) / (t1 - t0);
    vtkm::FloatDefault t2 = t * t, t3 = t2 * t;

    // Hermite basis functions.
    vtkm::FloatDefault h00 = 2 * t3 - 3 * t2 + 1;
    vtkm::FloatDefault h10 = t3 - 2 * t2 + t;
    vtkm::FloatDefault h01 = -2 * t3 + 3 * t2;
    vtkm::FloatDefault h11 = t3 - t2;

    const auto& d0 = this->Data.Get(idx);
    const auto& d1 = this->Data.Get(idx + 1);
    for (vtkm::Id i = 0; i < 3; ++i)
      val[i] = h00 * d0[i] + h10 * (t1 - t0) * m0[i] + h01 * d1[i] + h11 * (t1 - t0) * m1[i];

    return vtkm::ErrorCode::Success;
  }

private:
  vtkm::Id FindInterval(const vtkm::FloatDefault& t) const
  {
    vtkm::Id n = this->Knots.GetNumberOfValues();

    if (t < this->Knots.Get(0) || t > this->Knots.Get(n - 1))
      return -1;

    //Binary search for the interval
    vtkm::Id left = 0;
    vtkm::Id right = n - 1;

    while (left < right)
    {
      vtkm::Id mid = left + (right - left) / 2;

      if (t >= this->Knots.Get(mid) && t <= this->Knots.Get(mid + 1))
        return mid;
      else if (t < this->Knots.Get(mid))
        right = mid;
      else
        left = mid;
    }

    // t not within the interval. We should not get here.
    return -1;
  }

  DataPortalType Data;
  KnotsPortalType Knots;
  TangentPortalType Tangents;
};
} //namespace exec
} //namespace vtkm

#endif //vtk_m_exec_CubicHermiteSpline_h
