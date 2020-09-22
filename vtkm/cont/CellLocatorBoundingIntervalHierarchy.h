//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h
#define vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/cont/internal/CellLocatorBase.h>

#include <vtkm/exec/CellLocatorBoundingIntervalHierarchy.h>
#include <vtkm/exec/CellLocatorMultiplexer.h>

#include <vtkm/worklet/spatialstructure/BoundingIntervalHierarchy.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellLocatorBoundingIntervalHierarchy
  : public vtkm::cont::internal::CellLocatorBase<CellLocatorBoundingIntervalHierarchy>
{
  using Superclass = vtkm::cont::internal::CellLocatorBase<CellLocatorBoundingIntervalHierarchy>;

  template <typename Device>
  struct CellSetContToExec
  {
    template <typename CellSetCont>
    using Transform =
      typename CellSetCont::template ExecutionTypes<Device,
                                                    vtkm::TopologyElementTagCell,
                                                    vtkm::TopologyElementTagPoint>::ExecObjectType;
  };

  template <typename Device>
  struct CellSetExecToCellLocatorExec
  {
    template <typename CellSetExec>
    using Transform = vtkm::exec::CellLocatorBoundingIntervalHierarchy<CellSetExec, Device>;
  };


public:
  using SupportedCellSets = VTKM_DEFAULT_CELL_SET_LIST;

  template <typename Device>
  using CellLocatorList =
    vtkm::ListTransform<VTKM_DEFAULT_CELL_SET_LIST,
                        CellSetExecToCellLocatorExec<Device>::template Transform>;

  template <typename Device>
  using ExecObjType = vtkm::ListApply<CellLocatorList<Device>, vtkm::exec::CellLocatorMultiplexer>;

  VTKM_CONT
  CellLocatorBoundingIntervalHierarchy(vtkm::IdComponent numPlanes = 4,
                                       vtkm::IdComponent maxLeafSize = 5)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Nodes()
    , ProcessedCellIds()
  {
  }

  VTKM_CONT
  void SetNumberOfSplittingPlanes(vtkm::IdComponent numPlanes)
  {
    this->NumPlanes = numPlanes;
    this->SetModified();
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfSplittingPlanes() { return this->NumPlanes; }

  VTKM_CONT
  void SetMaxLeafSize(vtkm::IdComponent maxLeafSize)
  {
    this->MaxLeafSize = maxLeafSize;
    this->SetModified();
  }

  VTKM_CONT
  vtkm::Id GetMaxLeafSize() { return this->MaxLeafSize; }

  // When all the arrays get updated to the new buffer style, the template for this
  // method can be removed and the implementation moved to CellLocatorRectilinearGrid.cxx.
  template <typename Device>
  VTKM_CONT ExecObjType<Device> PrepareForExecution(Device device, vtkm::cont::Token& token) const
  {
    ExecObjType<Device> execObject;
    this->GetCellSet().CastAndCall(MakeExecObject{}, device, token, *this, execObject);
    return execObject;
  }

private:
  vtkm::IdComponent NumPlanes;
  vtkm::IdComponent MaxLeafSize;
  vtkm::cont::ArrayHandle<vtkm::exec::CellLocatorBoundingIntervalHierarchyNode> Nodes;
  vtkm::cont::ArrayHandle<vtkm::Id> ProcessedCellIds;

  friend Superclass;
  VTKM_CONT void Build();

  // When all the arrays get updated to the new buffer style, the template for this
  // method can be removed and the implementation moved to CellLocatorRectilinearGrid.cxx.
  struct MakeExecObject
  {
    template <typename CellSetType, typename Device>
    VTKM_CONT void operator()(const CellSetType& cellSet,
                              Device,
                              vtkm::cont::Token& token,
                              const CellLocatorBoundingIntervalHierarchy& self,
                              ExecObjType<Device>& execObject) const
    {
      execObject = vtkm::exec::CellLocatorBoundingIntervalHierarchy<CellSetType, Device>(
        self.Nodes,
        self.ProcessedCellIds,
        cellSet,
        self.GetCoordinates().GetDataAsMultiplexer(),
        Device{},
        token);
    }
  };
};

} // namespace cont
} // namespace vtkm

#endif // vtk_m_cont_CellLocatorBoundingIntervalHierarchy_h
