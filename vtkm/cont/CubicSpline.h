//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CubicSpline_h
#define vtk_m_cont_CubicSpline_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/exec/CubicSpline.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CubicSpline : public vtkm::cont::ExecutionObjectBase
{
public:
  CubicSpline() = default;
  virtual ~CubicSpline() = default;

  VTKM_CONT void SetControlPoints(const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& controlPoints)
  {
    this->ControlPoints = controlPoints;
    this->SetModified();
  }
  VTKM_CONT void SetValues(const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& values)
  {
    this->Values = values;
    this->SetModified();
  }

  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::FloatDefault> GetControlPoints() const
  {
    return this->ControlPoints;
  }

  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::FloatDefault> GetValues() const { return this->Values; }

  VTKM_CONT void Update() const;

  VTKM_CONT vtkm::exec::CubicSpline PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                        vtkm::cont::Token& token) const;

private:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  void Build();

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> CalcH() const;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> CalcAlpha(
    const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& h) const;
  void CalcCoefficients(const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& H,
                        const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& Alpha);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> ControlPoints;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> Values;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> CoefficientsA;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> CoefficientsB;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> CoefficientsC;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> CoefficientsD;
  mutable bool Modified = true;
};

}
} // vtkm::cont

#endif //vtk_m_cont_CubicSpline_h
