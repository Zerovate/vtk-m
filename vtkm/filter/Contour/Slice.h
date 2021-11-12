//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Slice_h
#define vtk_m_filter_Slice_h

#include <vtkm/filter/Contour/Contour.h>
#include <vtkm/filter/Contour/vtkm_filter_contour_export.h>

#include <vtkm/ImplicitFunction.h>

namespace vtkm
{
namespace filter
{

class VTKM_FILTER_CONTOUR_EXPORT Slice : public vtkm::filter::Contour
{
public:
  /// Set/Get the implicit function that is used to perform the slicing.
  ///
  VTKM_CONT
  void SetImplicitFunction(const vtkm::ImplicitFunctionGeneral& func) { this->Function = func; }
  VTKM_CONT
  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

private:
  vtkm::ImplicitFunctionGeneral Function;
};

}
} // vtkm::filter

#endif // vtk_m_filter_Slice_h
