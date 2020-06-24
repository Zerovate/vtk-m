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

#include "../Diffusion_Filter.h"
#include <iostream>


using namespace std;

#define DIFFUSE_COEFF 0.6f
#define X 10
#define Y 15



vtkm::cont::DataSet create_Data_Set(vtkm::Id2 dimensions,
                                    vtkm::Float32 tInside,
                                    vtkm::Float32 tBoundary)
{

  vtkm::cont::DataSet dataSet = vtkm::cont::DataSetBuilderUniform::Create(
    dimensions,
    vtkm::Vec2f{ -2.0f, -2.0f },
    vtkm::Vec2f{ 4.0f / dimensions[0], 4.0f / dimensions[1] });
  vtkm::cont::CoordinateSystem coords = dataSet.GetCoordinateSystem("coords");
  vtkm::cont::DataSetFieldAdd dataSetFieldAdd;
  std::vector<vtkm::Int8> boundary;
  std::vector<vtkm::Float32> temperature;
  std::vector<vtkm::Float32> diffuseCoeff;

  for (vtkm::Id i = 0; i < coords.GetNumberOfPoints(); ++i)
  {
    if (i < dimensions[0] || i > coords.GetNumberOfPoints() - dimensions[0] - 1 ||
        i % dimensions[0] == 0 || i % dimensions[0] == (dimensions[0] - 1))
    {
      temperature.push_back(tBoundary);
      boundary.push_back(DERICHLET);
      diffuseCoeff.push_back(DIFFUSE_COEFF);
    }
    else
    {

      temperature.push_back(tInside);
      boundary.push_back(NEUMMAN);
      diffuseCoeff.push_back(DIFFUSE_COEFF);
    }
  }


  dataSetFieldAdd.AddPointField(dataSet, "boundary_condition", boundary);
  dataSetFieldAdd.AddPointField(dataSet, "temperature", temperature);
  dataSetFieldAdd.AddPointField(dataSet, "coeff_diffusion", diffuseCoeff);
  return dataSet;
}

vtkm::cont::DataSet* gData = nullptr;
vtkm::filter::Diffusion* gFilter = nullptr;

int main(int argc, char* argv[])
{

  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);
  vtkm::cont::ArrayHandle<vtkm::Float32> copie;

  vtkm::cont::DataSet data;
  vtkm::Id2 dimensions(Y, X);
  data = create_Data_Set(dimensions, 5, 40);

  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(config.Device);

  vtkm::filter::Diffusion filter;
  gData = &data;
  gFilter = &filter;
  std::vector<int> ite;
  ite.push_back(1);

  vtkm::cont::DataSetFieldAdd::AddPointField(*gData, "iteration", ite);

  vtkm::cont::DataSet oData = gFilter->Execute(*gData, DiffusionPolicy());

  *gData = oData;

  //get the previous state of the matrix
  gData->GetPointField("temperature").GetData().CopyTo(copie);



  ifstream file("test_result_rectangle_height_data");
  int i = 0;
  vtkm::Float32 compare;

  bool equal = true;
  do
  {
    file >> compare;
    if (abs(compare - copie.ReadPortal().Get(i)) > 0.0001f)
    {
      equal = false;
    }
    i++;
  } while (equal && (i < X * Y));

  file.close();
  if (equal)

    return 0;
  else
    return 1;
}
