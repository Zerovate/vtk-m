//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/BinaryOperators.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/CubicHermiteSpline.h>
#include <vtkm/exec/CubicHermiteSpline.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{
namespace
{
struct CalcNeighborDistanceWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut, WholeArrayIn);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VTKM_EXEC void operator()(const vtkm::Id& idx,
                            vtkm::FloatDefault& val,
                            const ArrayType& data) const
  {
    if (idx == 0)
      val = 0.0;
    else
      val = vtkm::Magnitude(data.Get(idx) - data.Get(idx - 1));
  }
};

struct CalcTangentsWorklet : public vtkm::worklet::WorkletMapField
{
  CalcTangentsWorklet(const vtkm::Id& numPoints)
    : NumPoints(numPoints)
  {
  }

  using ControlSignature = void(FieldOut, WholeArrayIn, WholeArrayIn);
  using ExecutionSignature = void(InputIndex, _1, _2, _3);

  template <typename TangentArrayType, typename PointArrayType, typename KnotArrayType>
  VTKM_EXEC void operator()(const vtkm::Id& idx,
                            TangentArrayType& tangent,
                            const PointArrayType& points,
                            const KnotArrayType& knots) const
  {
    vtkm::Id idx0, idx1;
    if (idx == 0) // Forward difference
    {
      idx0 = 0;
      idx1 = 1;
    }
    else if (idx == NumPoints - 1) // Backward difference
    {
      idx0 = NumPoints - 2;
      idx1 = NumPoints - 1;
    }
    else // central difference
    {
      idx0 = idx - 1;
      idx1 = idx + 1;
    }

    auto dX = points.Get(idx1) - points.Get(idx0);
    auto dT = knots.Get(idx1) - knots.Get(idx0);

    tangent = dX / dT;
  }

  vtkm::Id NumPoints;
};
} //anonymous namespace

VTKM_CONT vtkm::Range CubicHermiteSpline::GetParametricRange()
{
  auto n = this->Knots.GetNumberOfValues();
  if (n == 0)
  {
    this->ComputeKnots();
    n = this->Knots.GetNumberOfValues();
  }
  const auto ids = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, n - 1 });
  const std::vector<vtkm::FloatDefault> output = vtkm::cont::ArrayGetValues(ids, this->Knots);
  return { output[0], output[1] };
}

VTKM_CONT void CubicHermiteSpline::ComputeKnots()
{
  vtkm::Id n = this->Data.GetNumberOfValues();
  this->Knots.Allocate(n);

  vtkm::cont::Invoker invoker;

  //uses chord length parameterization.
  invoker(CalcNeighborDistanceWorklet{}, this->Knots, this->Data);
  vtkm::FloatDefault sum = vtkm::cont::Algorithm::ScanInclusive(this->Knots, this->Knots);

  if (sum == 0.0)
    throw std::invalid_argument("Error: accumulated distance between data is zero.");

  auto divideBy = vtkm::cont::make_ArrayHandleConstant(1.0 / sum, this->Knots.GetNumberOfValues());
  vtkm::cont::Algorithm::Transform(this->Knots, divideBy, this->Knots, vtkm::Product{});
}

VTKM_CONT void CubicHermiteSpline::ComputeTangents()
{
  vtkm::Id n = this->Data.GetNumberOfValues();
  this->Tangents.Allocate(n);

  vtkm::cont::Invoker invoker;

  invoker(CalcTangentsWorklet{ n }, this->Tangents, this->Data, this->Knots);
}

vtkm::exec::CubicHermiteSpline CubicHermiteSpline::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device,
  vtkm::cont::Token& token)
{
  vtkm::Id n = this->Data.GetNumberOfValues();
  if (n < 2)
    throw vtkm::cont::ErrorBadValue("At least two points are required for spline interpolation.");
  if (this->Knots.GetNumberOfValues() == 0)
    this->ComputeKnots();
  if (this->Tangents.GetNumberOfValues() == 0)
    this->ComputeTangents();

  if (n != this->Knots.GetNumberOfValues())
    throw vtkm::cont::ErrorBadValue("Number of data points must match the number of knots.");
  if (n != this->Tangents.GetNumberOfValues())
    throw vtkm::cont::ErrorBadValue("Number of data points must match the number of tangents.");

  using ExecObjType = vtkm::exec::CubicHermiteSpline;

  return ExecObjType(this->Data, this->Knots, this->Tangents, device, token);
}
}
} // namespace vtkm::cont
