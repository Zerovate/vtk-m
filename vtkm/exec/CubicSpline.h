//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_exec_cubicspline_h
#define vtkm_exec_cubicspline_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{

namespace exec
{
class VTKM_ALWAYS_EXPORT CubicSpline
{
private:
  using ArrayHandleType = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using PortalType = typename ArrayHandleType::ReadPortalType;

public:
  VTKM_CONT CubicSpline(const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& controlPoints,
                        const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& values,
                        const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& coefficientsB,
                        const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& coefficientsC,
                        const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& coefficientsD,
                        vtkm::cont::DeviceAdapterId device,
                        vtkm::cont::Token& token)
  {
    this->ControlPointsPortal = controlPoints.PrepareForInput(device, token);
    this->ValuesPortal = values.PrepareForInput(device, token);
    this->CoefficientsBPortal = coefficientsB.PrepareForInput(device, token);
    this->CoefficientsCPortal = coefficientsC.PrepareForInput(device, token);
    this->CoefficientsDPortal = coefficientsD.PrepareForInput(device, token);
  }

  VTKM_EXEC
  bool Evaluate(const vtkm::FloatDefault& param, vtkm::FloatDefault& val) const
  {
    val = 0;

    vtkm::Id idx = this->FindInterval(param);
    if (idx < 0)
      return false;

    vtkm::FloatDefault dx = param - this->ControlPointsPortal.Get(idx);
    const auto B = this->CoefficientsBPortal.Get(idx);
    const auto C = this->CoefficientsCPortal.Get(idx);
    const auto D = this->CoefficientsDPortal.Get(idx);
    val = this->ValuesPortal.Get(idx) + B * dx + C * dx * dx + D * dx * dx * dx;

    return true;
  }

private:
  vtkm::Id FindInterval(const vtkm::FloatDefault& x) const
  {
    vtkm::Id numPoints = this->ControlPointsPortal.GetNumberOfValues();
    if (x < this->ControlPointsPortal.Get(0) || x > this->ControlPointsPortal.Get(numPoints - 1))
      return -1;

    for (vtkm::Id i = 0; i < numPoints - 1; ++i)
    {
      if (x >= this->ControlPointsPortal.Get(i) && x <= this->ControlPointsPortal.Get(i + 1))
        return i;
    }
    return -1;
  }

  PortalType ControlPointsPortal;
  PortalType ValuesPortal;
  PortalType CoefficientsBPortal;
  PortalType CoefficientsCPortal;
  PortalType CoefficientsDPortal;
};
} //namespace exec
} //namespace vtkm

#endif //vtkm_exec_cubicspline_h
