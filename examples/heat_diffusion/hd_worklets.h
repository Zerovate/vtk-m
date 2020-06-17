#ifndef HD_WORKLET_H
#define HD_WORKLET_H
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/filter/FieldToColors.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include "parameters.h"

#define NEUMMAN 0
#define DERICHLET 1

struct UpdateHeat : public vtkm::worklet::WorkletPointNeighborhood
{
  using CountingHandle = vtkm::cont::ArrayHandleCounting<vtkm::Id>;

  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood prevstate,
                                FieldIn condition,
                                FieldIn diffuseCoeff,
                                FieldOut state);

  using ExecutionSignature = void(_2, _3, _4, _5);

  template <typename NeighIn>
  VTKM_EXEC void operator()(const NeighIn& prevstate,
                            const vtkm::Int8& condition,
                            const vtkm::Float32& diffuseCoeff,
                            vtkm::Float32& state) const
  {

    auto current = prevstate.Get(0, 0, 0);

    if (condition == NEUMMAN)
    {
      state = diffuseCoeff * current +
        (1 - diffuseCoeff) * (0.25f * (prevstate.Get(-1, 0, 0) + prevstate.Get(0, -1, 0) +
                                       prevstate.Get(0, 1, 0) + prevstate.Get(1, 0, 0)));
    }
    else if (condition == DERICHLET)
    {
      state = current;
    }
  }
};


struct FillInitialCondition : public vtkm::worklet::WorkletMapField
{

  Parameters parameters;

  FillInitialCondition(const Parameters& params)
    : parameters(params)
  {
  }

  using ControlSignature = void(FieldIn coord,
                                FieldOut boundary,
                                FieldOut temperature,
                                FieldOut diffusion);

  using ExecutionSignature = void(_1, _2, _3, _4);
  VTKM_EXEC void operator()(vtkm::Vec<float, 3> coord,
                            vtkm::Int8& boundary,
                            vtkm::Float32& temperature,
                            vtkm::Float32& diffusion) const
  {
    if (coord[0] == -2.0f || coord[0] == 2.0f || coord[1] == -2.0f || coord[1] == 2.0f)
    {
      temperature = std::get<0>(parameters.temperature);
      boundary = DERICHLET;
      diffusion = parameters.diffuse_coeff;
    }
    else
    {

      float dx = coord[0] - 0.0f;
      float dy = coord[1] - 0.0f;
      float distance = dx * dx + dy * dy;

      float rayon = (1.25f * 1.25f);

      if (distance - rayon < 0.1f && distance - rayon > -0.1f)
      {
        temperature = std::get<0>(parameters.temperature);
        boundary = DERICHLET;
        diffusion = parameters.diffuse_coeff;
      }
      else
      {

        temperature = std::get<1>(parameters.temperature);
        boundary = NEUMMAN;
        diffusion = parameters.diffuse_coeff;
      }
    }
  }
};

#endif //UPDATEHEAT_WORKLET_H
