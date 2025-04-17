//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CubicHermiteSpline_h
#define vtk_m_cont_CubicHermiteSpline_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Range.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/UnknownArrayHandle.h>
#include <vtkm/exec/CubicHermiteSpline.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CubicHermiteSpline : public vtkm::cont::ExecutionObjectBase
{
public:
  CubicHermiteSpline() = default;
  virtual ~CubicHermiteSpline() = default;

  VTKM_CONT void SetData(const vtkm::cont::ArrayHandle<vtkm::Vec3f>& data) { this->Data = data; }
  VTKM_CONT void SetData(const std::vector<vtkm::Vec3f>& data,
                         vtkm::CopyFlag copy = vtkm::CopyFlag::On)
  {
    this->Data = vtkm::cont::make_ArrayHandle(data, copy);
  }

  VTKM_CONT void SetKnots(const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& knots)
  {
    this->Knots = knots;
  }
  VTKM_CONT void SetKnots(const std::vector<vtkm::FloatDefault>& knots,
                          vtkm::CopyFlag copy = vtkm::CopyFlag::On)
  {
    this->Knots = vtkm::cont::make_ArrayHandle(knots, copy);
  }

  VTKM_CONT void SetTangents(const vtkm::cont::ArrayHandle<vtkm::Vec3f>& tangents)
  {
    this->Tangents = tangents;
  }
  VTKM_CONT void SetTangents(const std::vector<vtkm::Vec3f>& tangents,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::On)
  {
    this->Tangents = vtkm::cont::make_ArrayHandle(tangents, copy);
  }

  VTKM_CONT vtkm::Range GetParametricRange();

  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Vec3f>& GetData() const { return this->Data; }
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Vec3f>& GetTangents()
  {
    if (this->Tangents.GetNumberOfValues() == 0)
      this->ComputeTangents();
    return this->Tangents;
  }
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& GetKnots()
  {
    if (this->Knots.GetNumberOfValues() == 0)
      this->ComputeKnots();
    return this->Knots;
  }

  VTKM_CONT vtkm::exec::CubicHermiteSpline PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                               vtkm::cont::Token& token);

private:
  VTKM_CONT void ComputeKnots();
  VTKM_CONT void ComputeTangents();

  vtkm::cont::ArrayHandle<vtkm::Vec3f> Data;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> Knots;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> Tangents;
};
}
} // vtkm::cont

#endif //vtk_m_cont_CubicHermiteSpline_h
