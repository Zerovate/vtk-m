//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_worklet_TemporalGridEvaluators_h
#define vtk_m_filter_flow_worklet_TemporalGridEvaluators_h

#include <vtkm/filter/flow/worklet/GridEvaluatorStatus.h>
#include <vtkm/filter/flow/worklet/GridEvaluators.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename FieldType, typename CellSetType>
class ExecutionTemporalGridEvaluator
{
private:
  using GridEvaluator = vtkm::worklet::flow::GridEvaluator<FieldType, CellSetType>;
  using ExecutionGridEvaluator =
    vtkm::worklet::flow::ExecutionGridEvaluator<FieldType, CellSetType>;

public:
  VTKM_CONT
  ExecutionTemporalGridEvaluator() = default;

  VTKM_CONT
  ExecutionTemporalGridEvaluator(const GridEvaluator& evaluatorOne,
                                 const vtkm::FloatDefault timeOne,
                                 const GridEvaluator& evaluatorTwo,
                                 const vtkm::FloatDefault timeTwo,
                                 vtkm::cont::DeviceAdapterId device,
                                 vtkm::cont::Token& token)
    : EvaluatorOne(evaluatorOne.PrepareForExecution(device, token))
    , EvaluatorTwo(evaluatorTwo.PrepareForExecution(device, token))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
    , TimeDiff(timeTwo - timeOne)
  {
  }

  template <typename Point>
  VTKM_EXEC bool IsWithinSpatialBoundary(const Point point) const
  {
    return this->EvaluatorOne.IsWithinSpatialBoundary(point) &&
      this->EvaluatorTwo.IsWithinSpatialBoundary(point);
  }

  VTKM_EXEC
  bool IsWithinTemporalBoundary(const vtkm::FloatDefault time) const
  {
    return time >= this->TimeOne && time <= this->TimeTwo;
  }

  VTKM_EXEC
  vtkm::Bounds GetSpatialBoundary() const { return this->EvaluatorTwo.GetSpatialBoundary(); }

  VTKM_EXEC_CONT
  vtkm::FloatDefault GetTemporalBoundary(vtkm::Id direction) const
  {
    return direction > 0 ? this->TimeTwo : this->TimeOne;
  }

  template <typename Point, typename ForceVectors>
  VTKM_EXEC GridEvaluatorStatus Evaluate(const Point& particle,
                                         vtkm::FloatDefault time,
                                         ForceVectors& out) const
  {
    // Validate time is in bounds for the current two slices.
    GridEvaluatorStatus status;

    if (!(time >= TimeOne && time <= TimeTwo))
    {
      status.SetFail();
      status.SetTemporalBounds();
      return status;
    }

    ForceVectors e1, e2;
    status = this->EvaluatorOne.Evaluate(particle, time, e1);
    if (status.CheckFail())
      return status;
    status = this->EvaluatorTwo.Evaluate(particle, time, e2);
    if (status.CheckFail())
      return status;

    // LERP between the two values of calculated fields to obtain the new value
    vtkm::FloatDefault proportion = (time - this->TimeOne) / this->TimeDiff;
    out = vtkm::Lerp(e1, e2, proportion);

    status.SetOk();
    return status;
  }

private:
  ExecutionGridEvaluator EvaluatorOne;
  ExecutionGridEvaluator EvaluatorTwo;
  vtkm::FloatDefault TimeOne;
  vtkm::FloatDefault TimeTwo;
  vtkm::FloatDefault TimeDiff;
};

template <typename FieldType, typename CellSetType>
class TemporalGridEvaluator : public vtkm::cont::ExecutionObjectBase
{
private:
  using GridEvaluator = vtkm::worklet::flow::GridEvaluator<FieldType, CellSetType>;

public:
  VTKM_CONT TemporalGridEvaluator() = default;

  VTKM_CONT TemporalGridEvaluator(const vtkm::cont::DataSet& ds1,
                                  const vtkm::FloatDefault t1,
                                  const FieldType& field1,
                                  const vtkm::cont::DataSet& ds2,
                                  const vtkm::FloatDefault t2,
                                  const FieldType& field2)
    : EvaluatorOne(GridEvaluator(ds1, field1))
    , EvaluatorTwo(GridEvaluator(ds2, field2))
    , TimeOne(t1)
    , TimeTwo(t2)
  {
  }


  VTKM_CONT TemporalGridEvaluator(GridEvaluator& evaluatorOne,
                                  const vtkm::FloatDefault timeOne,
                                  GridEvaluator& evaluatorTwo,
                                  const vtkm::FloatDefault timeTwo)
    : EvaluatorOne(evaluatorOne)
    , EvaluatorTwo(evaluatorTwo)
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  VTKM_CONT TemporalGridEvaluator(const vtkm::cont::CoordinateSystem& coordinatesOne,
                                  const vtkm::cont::UnknownCellSet& cellsetOne,
                                  const FieldType& fieldOne,
                                  const vtkm::FloatDefault timeOne,
                                  const vtkm::cont::CoordinateSystem& coordinatesTwo,
                                  const vtkm::cont::UnknownCellSet& cellsetTwo,
                                  const FieldType& fieldTwo,
                                  const vtkm::FloatDefault timeTwo)
    : EvaluatorOne(GridEvaluator(coordinatesOne, cellsetOne, fieldOne))
    , EvaluatorTwo(GridEvaluator(coordinatesTwo, cellsetTwo, fieldTwo))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  VTKM_CONT ExecutionTemporalGridEvaluator<FieldType, CellSetType> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return ExecutionTemporalGridEvaluator<FieldType, CellSetType>(
      this->EvaluatorOne, this->TimeOne, this->EvaluatorTwo, this->TimeTwo, device, token);
  }

private:
  GridEvaluator EvaluatorOne;
  GridEvaluator EvaluatorTwo;
  vtkm::FloatDefault TimeOne;
  vtkm::FloatDefault TimeTwo;
};

// Given the information for evaluators at 2 time steps, constructs a temporal grid
// evaluator of the appropriate type and calls the provided functor. This only
// works if the datasets both have the same types.
template <typename FieldType, typename Functor>
void CastAndCallTemporalGridEvaluator(
  Functor&& functor,
  const vtkm::cont::CoordinateSystem& coords1,
  const vtkm::cont::CoordinateSystem& coords2,
  const vtkm::cont::UnknownCellSet& cells1,
  const vtkm::cont::UnknownCellSet& cells2,
  const FieldType& field1,
  const FieldType& field2,
  vtkm::FloatDefault time1,
  vtkm::FloatDefault time2,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& ghostCells1 = vtkm::cont::ArrayHandle<vtkm::UInt8>{},
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& ghostCells2 = vtkm::cont::ArrayHandle<vtkm::UInt8>{})
{
  auto constructTemporal = [&](auto steadyGridEval1) {
    using SteadyGridEval = decltype(steadyGridEval1);
    using UnsteadyGridEval = TemporalGridEvaluator<FieldType, typename SteadyGridEval::CellSetType>;
    SteadyGridEval steadyGridEval2(coords2, cells2, field2, ghostCells2);
    UnsteadyGridEval unsteadyGridEval(steadyGridEval1, time1, steadyGridEval2, time2);
    functor(unsteadyGridEval);
  };
  vtkm::worklet::flow::CastAndCallGridEvaluator(
    constructTemporal, coords1, cells1, field1, ghostCells1);
}

template <typename FieldType, typename Functor>
void CastAndCallTemporalGridEvaluator(Functor&& functor,
                                      const vtkm::cont::DataSet& dataset1,
                                      const vtkm::cont::DataSet& dataset2,
                                      const FieldType& field1,
                                      const FieldType& field2,
                                      vtkm::FloatDefault time1,
                                      vtkm::FloatDefault time2,
                                      vtkm::IdComponent activeCoordinates1 = 0,
                                      vtkm::IdComponent activeCoordinates2 = 0)
{
  vtkm::cont::ArrayHandle<vtkm::UInt8> ghostArray1;
  if (dataset1.HasGhostCellField())
  {
    vtkm::cont::ArrayCopyShallowIfPossible(dataset1.GetGhostCellField().GetData(), ghostArray1);
  }
  vtkm::cont::ArrayHandle<vtkm::UInt8> ghostArray2;
  if (dataset2.HasGhostCellField())
  {
    vtkm::cont::ArrayCopyShallowIfPossible(dataset2.GetGhostCellField().GetData(), ghostArray2);
  }

  CastAndCallTemporalGridEvaluator(std::forward<Functor>(functor),
                                   dataset1.GetCoordinateSystem(activeCoordinates1),
                                   dataset2.GetCoordinateSystem(activeCoordinates2),
                                   dataset1.GetCellSet(),
                                   dataset2.GetCellSet(),
                                   field1,
                                   field2,
                                   time1,
                                   time2,
                                   ghostArray1,
                                   ghostArray2);
}

}
}
} //vtkm::worklet::flow

#endif // vtk_m_filter_flow_worklet_TemporalGridEvaluators_h
