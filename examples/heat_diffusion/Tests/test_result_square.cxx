//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>
#include <vtkm/filter/FilterDataSet.h>

#include "../Diffusion.hpp"
#include "../InitalCondition.h"
#include "../parameters.h"
#include <iostream>

using namespace std;

#define DIFFUSE_COEFF 0.6f
#define X 10

int main(int argc, char* argv[])
{

  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);
  vtkm::cont::ArrayHandle<vtkm::Float32> copie;

  vtkm::cont::DataSet data;
  Parameters params;

  params.dimension = X;
  params.diffuse_coeff = DIFFUSE_COEFF;
  params.iteration = 1;
  std::get<0>(params.temperature) = 40;
  std::get<1>(params.temperature) = 5;

  data = initial_condition(params);

  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(config.Device);

  vtkm::filter::Diffusion filter;
  std::vector<int> ite;
  ite.push_back(params.iteration);

  data.AddField(vtkm::cont::make_FieldPoint("iteration", vtkm::cont::make_ArrayHandle(ite)));


  data = filter.Execute(data);

  data.GetPointField("temperature").GetData().CopyTo(copie);

  ifstream file("test_result_square_data.txt");
  int i = 0;
  vtkm::Float32 compare;

  auto portal = copie.ReadPortal();

  bool equal = true;
  do
  {
    file >> compare;

    if (abs(compare - portal.Get(i)) > 0.0001f)
    {
      equal = false;
    }
    i++;
  } while (equal && (i < X * X));

  file.close();
  if (equal)

    return 0;
  else
    return 1;
}
