//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_cont_CellLocatorRectilinearGrid_h
#define vtkm_cont_CellLocatorRectilinearGrid_h

#include <vtkm/cont/internal/CellLocatorBase.h>

#include <vtkm/exec/CellLocatorRectilinearGrid.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellLocatorRectilinearGrid
  : public vtkm::cont::internal::CellLocatorBase<CellLocatorRectilinearGrid>
{
  using Superclass = vtkm::cont::internal::CellLocatorBase<CellLocatorRectilinearGrid>;

  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;
  // Might want to handle cartesian product of both Float32 and Float64.
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

public:
  CellLocatorRectilinearGrid() = default;

  ~CellLocatorRectilinearGrid() = default;

  // When all the arrays get updated to the new buffer style, the template for this
  // method can be removed and the implementation moved to CellLocatorRectilinearGrid.cxx.
  template <typename DeviceAdapter>
  VTKM_CONT vtkm::exec::CellLocatorRectilinearGrid<DeviceAdapter> PrepareForExecution(
    DeviceAdapter device,
    vtkm::cont::Token& token) const
  {
    using ExecObjType = vtkm::exec::CellLocatorRectilinearGrid<DeviceAdapter>;

    if (this->Is3D)
    {
      return ExecObjType(this->PlaneSize,
                         this->RowSize,
                         this->GetCellSet().template Cast<Structured3DType>(),
                         this->GetCoordinates().GetData().template Cast<RectilinearType>(),
                         device,
                         token);
    }
    else
    {
      return ExecObjType(this->PlaneSize,
                         this->RowSize,
                         this->GetCellSet().template Cast<Structured2DType>(),
                         this->GetCoordinates().GetData().template Cast<RectilinearType>(),
                         device,
                         token);
    }
  }

private:
  vtkm::Bounds Bounds;
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;
  bool Is3D = true;

  friend Superclass;
  VTKM_CONT void Build();
};

} //namespace cont
} //namespace vtkm

#endif //vtkm_cont_CellLocatorRectilinearGrid_h
