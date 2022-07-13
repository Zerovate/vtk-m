//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_MapFieldEdge_Interpolation_h
#define vtk_m_worklet_MapFieldEdge_Interpolation_h

#include "vtkm/VectorAnalysis.h"
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{

struct EdgeInterpolation
{
  vtkm::Id Vertex1 = -1;
  vtkm::Id Vertex2 = -1;
  vtkm::Float64 Weight = 0;

  VTKM_EXEC_CONT
  bool operator<(const EdgeInterpolation& that) const
  {
    return (this->Vertex1 < that.Vertex1) ||
      (this->Vertex1 == that.Vertex1 && this->Vertex2 < that.Vertex2);
  }

  VTKM_EXEC_CONT
  bool operator==(const EdgeInterpolation& that) const
  {
    return this->Vertex1 == that.Vertex1 && this->Vertex2 == that.Vertex2;
  }
};

namespace internal
{

template <typename T>
VTKM_EXEC_CONT T Scale(const T& val, vtkm::Float64 scale)
{
  return static_cast<T>(scale * static_cast<vtkm::Float64>(val));
}

template <typename T, vtkm::IdComponent NumComponents>
VTKM_EXEC_CONT vtkm::Vec<T, NumComponents> Scale(const vtkm::Vec<T, NumComponents>& val,
                                                 vtkm::Float64 scale)
{
  return val * scale;
}

} // namespace internal

class PerformEdgeInterpolations : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn edgeInterpolations,
                                WholeArrayIn inputField,
                                FieldOut outputField);

  using ExecutionSignature = void(_1, _2, _3);

  template <typename EdgeInterp, typename InputFieldPortal, typename OutputFieldType>
  VTKM_EXEC void operator()(const EdgeInterp& ei,
                            InputFieldPortal& inField,
                            OutputFieldType& outField) const
  {
    if (ei.Weight > vtkm::Float64(1) || ei.Weight < vtkm::Float64(0))
    {
      this->RaiseError("Error in edge weight, assigned value not in interval [0,1].");
    }
    outField = static_cast<OutputFieldType>(
      vtkm::Lerp(inField.Get(ei.Vertex1), inField.Get(ei.Vertex2), ei.Weight));
  }
};

} // namespace worklet

//namespace filter
//{
//VTKM_CONT bool MapArrayEdgeInterpolation(
//  const vtkm::cont::UnknownArrayHandle& inputArray,
//  const vtkm::cont::ArrayHandle<worklet::EdgeInterpolation> edgeInterpolations,
//  const vtkm::cont::UnknownArrayHandle& outputArray
//  )
//{
//  auto DoMapFieldEdgeInterpolation = [&](const auto& a, const auto& b)
//  {
//    vtkm::worklet::PerformEdgeInterpolations edgeInterpWorklet;
//
//    vtkm::worklet::DispatcherMapField<worklet::PerformEdgeInterpolations> edgeInterpDispatcher(
//      edgeInterpWorklet);
//    edgeInterpDispatcher.Invoke(edgeInterpolations, a, b);
//  };
//
//  inputArray.CastAndCallWithExtractedArray(DoMapFieldEdgeInterpolation, outputArray);
//  return true;
//}
//} // namespace filter
} // namespace vtkm
#endif //vtk_m_worklet_MapFieldEdge_Interpolation_h
