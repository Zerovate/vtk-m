//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef DIFFUSION_FILTER_H
#define DIFFUSION_FILTER_H

#include "hd_worklets.h"

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <utility>

namespace vtkm
{
namespace filter
{

class Diffusion : public vtkm::filter::FilterDataSet<Diffusion>
{
public:
  using SupportedTypes = vtkm::List<vtkm::Float32, vtkm::Int8, int, vtkm::Vec3f_32>;

  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<Policy> policy)
  {

    vtkm::cont::ArrayHandle<vtkm::Float32> state;
    vtkm::cont::ArrayHandle<vtkm::Float32> prevstate;
    vtkm::cont::ArrayHandle<vtkm::Float32> diffusionCoeff;
    vtkm::cont::ArrayHandle<vtkm::Int8> condition;
    vtkm::cont::ArrayHandle<int> iteration;

    //get the coordinate system we are using for the area
    const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

    //get the previous state of the matrix
    input.GetPointField("boundary_condition").GetData().CopyTo(condition);
    input.GetPointField("temperature").GetData().CopyTo(prevstate);
    input.GetPointField("coeff_diffusion").GetData().CopyTo(diffusionCoeff);
    input.GetPointField("iteration").GetData().CopyTo(iteration);

    vtkm::cont::ArrayHandle<vtkm::Float32> *ptra = &prevstate, *ptrb = &state;
    //Update the temperature
    for (int i = 0; i < iteration.ReadPortal().Get(0); i++)
    {
      this->Invoke(UpdateHeat{},
                   vtkm::filter::ApplyPolicyCellSet(cells, policy, *this),
                   *ptra,
                   condition,
                   diffusionCoeff,
                   *ptrb);
      std::swap(ptra, ptrb);
    }

    //save the results
    vtkm::cont::DataSet output;
    output.CopyStructure(input);

    output.AddField(vtkm::cont::make_FieldPoint("coeff_diffusion", diffusionCoeff));
    output.AddField(vtkm::cont::make_FieldPoint("boundary_condition", condition));
    output.AddField(vtkm::cont::make_FieldPoint("temperature", state));
    output.AddField(vtkm::cont::make_FieldPoint("iteration", iteration));

    return output;
  }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet&,
                            const vtkm::cont::ArrayHandle<T, StorageType>&,
                            const vtkm::filter::FieldMetadata&,
                            vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    return false;
  }
};
}
}

#endif //DIFFUSION_FILTER_H
