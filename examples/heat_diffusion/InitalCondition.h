//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef HEAT_DIFFUSION_INITALCONDITION_H
#define HEAT_DIFFUSION_INITALCONDITION_H

#include <utility>

#include "parameters.h"
struct FillInitialCondition : public vtkm::worklet::WorkletMapField
{

  Parameters parameters;

  explicit FillInitialCondition(Parameters params)
    : parameters(std::move(params))
  {
  }

  using ControlSignature = void(FieldIn, FieldOut, FieldOut, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4);
  template <typename CoordType>
  VTKM_EXEC void operator()(const CoordType& coord,
                            vtkm::UInt8& boundary,
                            vtkm::Float32& temperature,
                            vtkm::Float32& diffusion) const;
};
vtkm::cont::DataSet initial_condition(const Parameters& params);

#endif //HEAT_DIFFUSION_INITALCONDITION_H
