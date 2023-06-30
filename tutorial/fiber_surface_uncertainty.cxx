//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Example: uniform uncertainty visualization
//

#include <vtkm/cont/Initialize.h>
#include <vtkm/filter/uncertainty/Fiber.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>


int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  vtkm::io::VTKDataSetReader reader(
    "/Users/n5j/Desktop/supernova_visit_400_400_400_ReduceMinMax_Two_Variables.vtk");
  vtkm::cont::DataSet Data = reader.ReadDataSet();

  vtkm::filter::uncertainty::Fiber filter;
  std::vector<std::pair<double, double>> minAxisValues = { { 0.0, 0.0 } };
  std::vector<std::pair<double, double>> maxAxisValues = { { 0.2, 0.2 } };

  filter.SetMaxAxis(maxAxisValues);
  filter.SetMinAxis(minAxisValues);

  filter.SetMinOne("Iron_ensemble_min");
  filter.SetMaxOne("Iron_ensemble_max");
  filter.SetMinTwo("Nickel_ensemble_min");
  filter.SetMaxTwo("Nickel_ensemble_max");

  vtkm::cont::DataSet Output = filter.Execute(Data);
  vtkm::io::VTKDataSetWriter writer("/Users/n5j/Desktop/out_fiber_Two_Variable_supernova.vtk");
  writer.WriteDataSet(Output);

  return 0;
}
