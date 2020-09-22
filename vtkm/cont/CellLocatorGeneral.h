//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellLocatorGeneral_h
#define vtk_m_cont_CellLocatorGeneral_h

#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>

#include <vtkm/exec/CellLocatorMultiplexer.h>

#include <vtkm/cont/internal/Variant.h>

#include <functional>
#include <memory>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellLocatorGeneral
  : public vtkm::cont::internal::CellLocatorBase<CellLocatorGeneral>
{
  using Superclass = vtkm::cont::internal::CellLocatorBase<CellLocatorGeneral>;

public:
  using ContLocatorList = vtkm::List<vtkm::cont::CellLocatorUniformGrid,
                                     vtkm::cont::CellLocatorRectilinearGrid,
                                     vtkm::cont::CellLocatorTwoLevel>;

  template <typename Device>
  using ExecLocatorList = vtkm::List<
    vtkm::cont::internal::ExecutionObjectType<vtkm::cont::CellLocatorUniformGrid, Device>,
    vtkm::cont::internal::ExecutionObjectType<vtkm::cont::CellLocatorRectilinearGrid, Device>,
    vtkm::cont::internal::ExecutionObjectType<vtkm::cont::CellLocatorTwoLevel, Device>>;

  template <typename Device>
  using ExecObjType = vtkm::ListApply<ExecLocatorList<Device>, vtkm::exec::CellLocatorMultiplexer>;

private:
  vtkm::cont::internal::ListAsVariant<ContLocatorList> LocatorImpl;

  template <typename Device>
  struct PrepareFunctor
  {
    template <typename LocatorType>
    ExecObjType<Device> operator()(LocatorType&& locator,
                                   Device device,
                                   vtkm::cont::Token& token) const
    {
      return locator.PrepareForExecution(device, token);
    }
  };

public:
  // When all the arrays get updated to the new buffer style, the template for this
  // method can be removed and the implementation moved to CellLocatorRectilinearGrid.cxx.
  template <typename Device>
  VTKM_CONT ExecObjType<Device> PrepareForExecution(Device device, vtkm::cont::Token& token) const
  {
    return this->LocatorImpl.CastAndCall(PrepareFunctor<Device>{}, device, token);
  }

private:
  friend Superclass;
  VTKM_CONT void Build();
};
}
} // vtkm::cont

#endif // vtk_m_cont_CellLocatorGeneral_h
