//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleIsMonotonic.h>
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

struct DivideBy : public vtkm::worklet::WorkletMapField
{
  DivideBy(const vtkm::FloatDefault& divisor)
    : Divisor(divisor)
  {
  }

  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  VTKM_EXEC void operator()(vtkm::FloatDefault& val) const { val = val / this->Divisor; }

  vtkm::FloatDefault Divisor;
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
    std::cout << "Tan: " << idx << " = " << tangent << "  dx: " << dX << " " << dT << std::endl;
  }

  vtkm::Id NumPoints;
};
} //anonymous namespace

VTKM_CONT vtkm::Range CubicHermiteSpline::GetParametricRange() const
{
  auto portal = this->Knots.ReadPortal();
  auto n = portal.GetNumberOfValues();
  return { portal.Get(0), portal.Get(n - 1) };
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

  invoker(DivideBy{ sum }, this->Knots);
  std::cout << ":: params= ";
  vtkm::cont::printSummary_ArrayHandle(this->Knots, std::cout);
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
  vtkm::cont::Token& token) const
{
  vtkm::Id n = this->Data.GetNumberOfValues();
  if (n < 2)
    throw std::invalid_argument("At least two points are required for spline interpolation.");

  bool isMonotonic = vtkm::cont::IsMonotonicIncreasing(this->Knots);
  if (!isMonotonic)
    throw std::invalid_argument("Error. Knots must be monotonic increasing.");

  if (n != this->Knots.GetNumberOfValues())
    throw std::invalid_argument("Number of data points must match the number of knots.");
  if (n != this->Tangents.GetNumberOfValues())
    throw std::invalid_argument("Number of data points must match the number of tangents.");

  using ExecObjType = vtkm::exec::CubicHermiteSpline;

  return ExecObjType(this->Data, this->Knots, this->Tangents, device, token);
}

}
} // namespace vtkm::cont
