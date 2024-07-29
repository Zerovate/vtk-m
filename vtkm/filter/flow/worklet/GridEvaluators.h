//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_worklet_GridEvaluators_h
#define vtk_m_filter_flow_worklet_GridEvaluators_h

#include <vtkm/CellClassification.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocatorChooser.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DefaultTypes.h>

#include <vtkm/filter/flow/worklet/Field.h>
#include <vtkm/filter/flow/worklet/GridEvaluatorStatus.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename FieldType, typename CellSetType, typename CellLocatorType>
class ExecutionGridEvaluator
{
  using GhostCellType = vtkm::cont::ArrayHandle<vtkm::UInt8>::ReadPortalType;

public:
  VTKM_CONT
  ExecutionGridEvaluator() = default;

  VTKM_CONT
  ExecutionGridEvaluator(const CellLocatorType& locator,
                         const CellSetType cells,
                         const vtkm::Bounds& bounds,
                         const FieldType& field,
                         const GhostCellType& ghostCells,
                         bool haveGhostCells)
    : Bounds(bounds)
    , Field(field)
    , GhostCells(ghostCells)
    , HaveGhostCells(haveGhostCells)
    , Cells(cells)
    , Locator(locator)
  {
  }

  VTKM_EXEC
  bool IsWithinTemporalBoundary(const vtkm::FloatDefault& vtkmNotUsed(time)) const { return true; }

  VTKM_EXEC
  vtkm::Bounds GetSpatialBoundary() const { return this->Bounds; }

  VTKM_EXEC_CONT
  vtkm::FloatDefault GetTemporalBoundary(vtkm::Id direction) const
  {
    // Return the time of the newest time slice
    return direction > 0 ? vtkm::Infinity<vtkm::FloatDefault>()
                         : vtkm::NegativeInfinity<vtkm::FloatDefault>();
  }

private:
  template <typename Point, typename FlowVectors>
  VTKM_EXEC GridEvaluatorStatus HelpEvaluate(const Point& point,
                                             const vtkm::FloatDefault& time,
                                             FlowVectors& out) const
  {
    vtkm::Id cellId = -1;
    Point parametric;
    GridEvaluatorStatus status;

    status.SetOk();
    if (!this->IsWithinTemporalBoundary(time))
    {
      status.SetFail();
      status.SetTemporalBounds();
    }

    this->Locator.FindCell(point, cellId, parametric);
    if (cellId == -1)
    {
      status.SetFail();
      status.SetSpatialBounds();
    }
    else if (this->InGhostCell(cellId))
    {
      status.SetFail();
      status.SetInGhostCell();
      status.SetSpatialBounds();
    }

    //If initial checks ok, then do the evaluation.
    if (status.CheckOk())
    {
      if (this->Field.GetAssociation() == vtkm::cont::Field::Association::Points)
      {
        this->Field.GetValue(
          this->Cells.GetIndices(cellId), parametric, this->Cells.GetCellShape(cellId), out);
      }
      else if (this->Field.GetAssociation() == vtkm::cont::Field::Association::Cells)
      {
        this->Field.GetValue(cellId, out);
      }

      status.SetOk();
    }
    return status;
  }

  template <typename Point, typename FlowVectors>
  VTKM_EXEC GridEvaluatorStatus DeligateEvaluateToField(const Point& point,
                                                        const vtkm::FloatDefault& time,
                                                        FlowVectors& out) const
  {
    GridEvaluatorStatus status;
    status.SetOk();
    // TODO: Allow for getting status from deligated work from Field
    if (!this->Field.GetValue(point, time, out, this->Locator, this->Cells))
    {
      status.SetFail();
      status.SetSpatialBounds();
    }
    return status;
  }

  template <typename Point, typename FlowVectors>
  VTKM_EXEC GridEvaluatorStatus Evaluate(const Point& point,
                                         const vtkm::FloatDefault& time,
                                         FlowVectors& out,
                                         std::false_type vtkmNotUsed(delgateToField)) const
  {
    return this->HelpEvaluate(point, time, out);
  }

  template <typename Point, typename FlowVectors>
  VTKM_EXEC GridEvaluatorStatus Evaluate(const Point& point,
                                         const vtkm::FloatDefault& time,
                                         FlowVectors& out,
                                         std::true_type vtkmNotUsed(delgateToField)) const
  {
    return this->DeligateEvaluateToField(point, time, out);
  }

public:
  template <typename Point, typename FlowVectors>
  VTKM_EXEC GridEvaluatorStatus Evaluate(const Point& point,
                                         const vtkm::FloatDefault& time,
                                         FlowVectors& out) const
  {
    return this->Evaluate(point, time, out, typename FieldType::DelegateToField{});
  }

private:
  VTKM_EXEC bool InGhostCell(const vtkm::Id& cellId) const
  {
    if (this->HaveGhostCells && cellId != -1)
      return GhostCells.Get(cellId) == vtkm::CellClassification::Ghost;

    return false;
  }

  vtkm::Bounds Bounds;
  FieldType Field;
  GhostCellType GhostCells;
  bool HaveGhostCells;
  CellSetType Cells;
  CellLocatorType Locator;
};

template <typename FieldType_, typename CellSetType_, typename CellLocatorType_>
class GridEvaluator : public vtkm::cont::ExecutionObjectBase
{
public:
  using FieldType = FieldType_;
  using CellSetType = CellSetType_;
  using CellLocatorType = CellLocatorType_;
  using GhostCellArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8>;

  VTKM_CONT
  GridEvaluator() = default;

  VTKM_CONT
  GridEvaluator(const vtkm::cont::DataSet& dataSet, const FieldType& field)
    : Bounds(dataSet.GetCoordinateSystem().GetBounds())
    , Field(field)
    , GhostCellArray()
  {
    // This will throw an exception if the cell set is the wrong type.
    dataSet.GetCellSet().AsCellSet(this->Cells);

    this->InitializeLocator(dataSet.GetCoordinateSystem(), dataSet.GetCellSet());

    if (dataSet.HasGhostCellField())
    {
      auto arr = dataSet.GetGhostCellField().GetData();
      vtkm::cont::ArrayCopyShallowIfPossible(arr, this->GhostCellArray);
    }
  }

  VTKM_CONT
  GridEvaluator(const vtkm::cont::CoordinateSystem& coordinates,
                const vtkm::cont::UnknownCellSet& cellset,
                const FieldType& field,
                const GhostCellArrayType& ghostCellArray = GhostCellArrayType{})
    : Bounds(coordinates.GetBounds())
    , Field(field)
    , GhostCellArray(ghostCellArray)
  {
    this->InitializeLocator(coordinates, cellset);
  }

  VTKM_CONT auto PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const
  {
    auto locatorExec = this->Locator.PrepareForExecution(device, token);
    auto cellExec = this->Cells.PrepareForInput(
      device, vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{}, token);
    auto fieldExec = this->Field.PrepareForExecution(device, token);
    auto ghostExec = this->GhostCellArray.PrepareForInput(device, token);
    using ExecObjType =
      ExecutionGridEvaluator<decltype(fieldExec), decltype(cellExec), decltype(locatorExec)>;
    return ExecObjType(locatorExec,
                       cellExec,
                       this->Bounds,
                       fieldExec,
                       ghostExec,
                       this->GhostCellArray.GetNumberOfValues() > 0);
  }

private:
  VTKM_CONT void InitializeLocator(const vtkm::cont::CoordinateSystem& coordinates,
                                   const vtkm::cont::UnknownCellSet& cellset)
  {
    this->Locator.SetCoordinates(coordinates);
    this->Locator.SetCellSet(cellset);
    this->Locator.Update();
    cellset.AsCellSet(this->Cells);
  }

  vtkm::Bounds Bounds;
  FieldType Field;
  GhostCellArrayType GhostCellArray;
  CellSetType Cells;
  CellLocatorType Locator;
};

// Given coordinates, cell set, and flow field, constructs a grid evaluator of the
// appropriate type and calls the provided functor.
template <typename FieldType, typename Functor>
void CastAndCallGridEvaluator(Functor&& functor,
                              const vtkm::cont::CoordinateSystem& coords,
                              const vtkm::cont::UnknownCellSet& cells,
                              const FieldType& field,
                              const vtkm::cont::ArrayHandle<vtkm::UInt8>& ghostCellArray =
                                vtkm::cont::ArrayHandle<vtkm::UInt8>{})
{
  auto doLocator = [&](auto locator, auto cellSet) {
    using CellSetType = decltype(cellSet);
    using CellLocatorType = decltype(locator);
    vtkm::worklet::flow::GridEvaluator<FieldType, CellSetType, CellLocatorType> gridEvaluator(
      coords, cellSet, field, ghostCellArray);
    functor(gridEvaluator);
  };

  auto doType = [&](auto cellSet) {
    vtkm::cont::CastAndCallCellLocatorChooser(cellSet, coords, doLocator, cellSet);
  };

  cells.CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST>(doType);
}

template <typename FieldType, typename Functor>
void CastAndCallGridEvaluator(Functor&& functor,
                              const vtkm::cont::DataSet& dataset,
                              const FieldType& field,
                              vtkm::IdComponent activeCoordinates = 0)
{
  vtkm::cont::ArrayHandle<vtkm::UInt8> ghostArray;
  if (dataset.HasGhostCellField())
  {
    vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetGhostCellField().GetData(), ghostArray);
  }

  CastAndCallGridEvaluator(std::forward<Functor>(functor),
                           dataset.GetCoordinateSystem(activeCoordinates),
                           dataset.GetCellSet(),
                           field,
                           ghostArray);
}

}
}
} //vtkm::worklet::flow

#endif // vtk_m_filter_flow_worklet_GridEvaluators_h
