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

#include <vtkm/Pair.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/filter/uncertainty/FiberMultiVar.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>


int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  std::string fileName = argv[1];
  std::string outputName = argv[2];
  std::cout << "File Path/File Name" << fileName << std::endl;

  vtkm::io::VTKDataSetReader reader(fileName);
  vtkm::cont::DataSet Data = reader.ReadDataSet();

  vtkm::filter::uncertainty::FiberMultiVar filter;
  vtkm::Vec<vtkm::Float64, 3> bottomLeftValue = {0.2, 0.2, 0.2};
  vtkm::Vec<vtkm::Float64, 3> topRightValue = {0.3, 0.3, 0.3};

  filter.SetBottomLeftAxis(bottomLeftValue);
  filter.SetTopRightAxis(topRightValue);

  filter.SetMinX("Iron_ensemble_min");
  filter.SetMaxX("Iron_ensemble_max");
  filter.SetMinY("Nickel_ensemble_min");
  filter.SetMaxY("Nickel_ensemble_max");
  filter.SetMinZ("Iron_ensemble_min");
  filter.SetMaxZ("Iron_ensemble_max");

  vtkm::cont::DataSet Output = filter.Execute(Data);
  vtkm::io::VTKDataSetWriter writer(outputName);
  writer.WriteDataSet(Output);

  return 0;
}
