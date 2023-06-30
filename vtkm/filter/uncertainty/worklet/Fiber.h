//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// New Fiber.h


#ifndef vtk_m_worklet_uncertainty_Fiber_h
#define vtk_m_worklet_uncertainty_Fiber_h
#include <iostream>
#include <utility>
#include <vector>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

namespace vtkm
{
namespace worklet
{
namespace detail
{
class VTKM_FILTER_UNCERTAINTY_EXPORT Fiber : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  // Worklet Input
  Fiber(const std::vector<std::pair<double, double>>& minAxis,
        const std::vector<std::pair<double, double>>& maxAxis)
    : InputMinAxis(minAxis)
    , InputMaxAxis(maxAxis){};

  // Input and Output Parameters
  using ControlSignature = void(CellSetIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut, FieldOut);

  using ExecutionSignature = void(_2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  // Template
  template <typename MinOne,
            typename MaxOne,
            typename MinTwo,
            typename MaxTwo,
            typename OutCellFieldType1,
            typename OutCellFieldType2>
  // Operator
  VTKM_EXEC void operator()(const MinOne& EnsembleMinOne,
                            const MaxOne& EnsembleMaxOne,
                            const MinTwo& EnsembleMinTwo,
                            const MaxTwo& EnsembleMaxTwo,
                            OutCellFieldType1& OutputArea,
                            OutCellFieldType2& OutputProbablity) const
  {
    vtkm::FloatDefault X1 = static_cast<vtkm::FloatDefault>(InputMinAxis[0].first);  //(x1)
    vtkm::FloatDefault Y1 = static_cast<vtkm::FloatDefault>(InputMinAxis[0].second); //(y1)
    vtkm::FloatDefault X2 = static_cast<vtkm::FloatDefault>(InputMaxAxis[0].first);  //(x2)
    vtkm::FloatDefault Y2 = static_cast<vtkm::FloatDefault>(InputMaxAxis[0].second); //(y2)
    //std::cout << X1 << "," << Y1 << "," << X2 << "," << Y2 << std::endl;
    vtkm::FloatDefault TraitArea = (X2 - X1) * (Y2 - Y1);
    //std::cout << X2-X1 << "," << Y2-Y1 << ","<< TraitArea << std::endl;
    vtkm::FloatDefault X5 = 0.0;
    vtkm::FloatDefault X6 = 0.0;
    vtkm::FloatDefault Y5 = 0.0;
    vtkm::FloatDefault Y6 = 0.0;

    vtkm::FloatDefault X3 = 0.0;
    vtkm::FloatDefault Y3 = 0.0;
    vtkm::FloatDefault X4 = 0.0;
    vtkm::FloatDefault Y4 = 0.0;

    vtkm::FloatDefault IntersectionArea = 0.0;
    vtkm::FloatDefault IntersectionProbablity = 0.0;
    vtkm::FloatDefault IntersectionHeight = 0.0;
    vtkm::FloatDefault IntersectionWidth = 0.0;

    X3 = static_cast<vtkm::FloatDefault>(EnsembleMinOne); //(x3)
    X4 = static_cast<vtkm::FloatDefault>(EnsembleMaxOne); //(x4)
    Y3 = static_cast<vtkm::FloatDefault>(EnsembleMinTwo); //(y3)
    Y4 = static_cast<vtkm::FloatDefault>(EnsembleMaxTwo); //(y4)
    X5 = std::max(X1, X3);                                //(X1,X3)
    Y5 = std::max(Y1, Y3);                                //(y1,y3)
    X6 = std::min(X2, X4);                                //(X2,X4)
    Y6 = std::min(Y2, Y4);                                //(y2,y4)

    IntersectionHeight = Y6 - Y5;
    IntersectionWidth = X6 - X5;

    if ((IntersectionHeight > 0) and (IntersectionWidth > 0) and (X5 < X6) and (Y5 < Y6))
    {
      IntersectionArea = IntersectionHeight * IntersectionWidth;
      IntersectionProbablity = IntersectionArea / TraitArea;
    }
    OutputArea = IntersectionArea;
    OutputProbablity = IntersectionProbablity;
    return;
  }

private:
  std::vector<std::pair<double, double>> InputMinAxis;
  std::vector<std::pair<double, double>> InputMaxAxis;
};
}
}
}
#endif
