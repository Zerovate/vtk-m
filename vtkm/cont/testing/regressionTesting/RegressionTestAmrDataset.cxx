//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs = std::filesystem;

#include <vtkm/cont/MergePartitionedDataSet.h>
#include <vtkm/filter/ExternalFaces.h>
#include <vtkm/filter/Threshold.h>
#include <vtkm/source/Amr.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{

void TestAmrDatasetExecute(int dim, int numberOfLevels, int cellsPerDimension)
{
  std::cout << "Generate Image for AMR" << std::endl;

  std::cout << "Current path is " << fs::current_path() << '\n'; // (1)
  fs::current_path(fs::temp_directory_path());                   // (3)
  std::cout << "Current path is " << fs::current_path() << '\n';

  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  // Generate AMR
  vtkm::source::Amr source(dim, cellsPerDimension, numberOfLevels);
  vtkm::cont::AmrDataSet amrDataSet = source.Execute();

  // Remove blanked cells
  vtkm::filter::Threshold threshold;
  threshold.SetLowerThreshold(0);
  threshold.SetUpperThreshold(1);
  threshold.SetActiveField("vtkGhostType");
  vtkm::cont::PartitionedDataSet derivedDataSet = threshold.Execute(amrDataSet);

  // Extract surface for efficient 3D pipeline
  vtkm::filter::ExternalFaces surface;
  derivedDataSet = surface.Execute(derivedDataSet);

  // Merge dataset
  vtkm::cont::DataSet result = vtkm::cont::MergePartitionedDataSet(derivedDataSet);
  result.PrintSummary(std::cout);

  std::ifstream in1("./../../../../../vtk-m/data/baseline/cont/amr2D.png",
                    std::ifstream::ate | std::ifstream::binary);
  std::cout << "Hier baseline " << in1.tellg() << std::endl;
  std::ifstream in2("./../../../../../build/cont/test-amr2D.png",
                    std::ifstream::ate | std::ifstream::binary);
  std::cout << "Hier temp " << in2.tellg() << std::endl;
  std::ifstream in3("./../../../../../build/cont/diff-amr2D.png",
                    std::ifstream::ate | std::ifstream::binary);
  std::cout << "Hier diff " << in3.tellg() << std::endl;

  std::ifstream in4("./../../../../../vtk-m/data/baseline/cont/amr3D.png",
                    std::ifstream::ate | std::ifstream::binary);
  std::cout << "Hier baseline " << in4.tellg() << std::endl;
  std::ifstream in5("./../../../../../build/cont/test-amr3D.png",
                    std::ifstream::ate | std::ifstream::binary);
  std::cout << "Hier temp " << in5.tellg() << std::endl;
  std::ifstream in6("./../../../../../build/cont/diff-amr3D.png",
                    std::ifstream::ate | std::ifstream::binary);
  std::cout << "Hier diff " << in6.tellg() << std::endl;

  vtkm::rendering::testing::RenderAndRegressionTest<M, C, V3>(result,
                                                              "RTDataCells",
                                                              vtkm::cont::ColorTable("inferno"),
                                                              "cont/amr" + std::to_string(dim) +
                                                                "D.png",
                                                              false);
}

void TestAmrDataset()
{
  int numberOfLevels = 5;
  int cellsPerDimension = 6;
  TestAmrDatasetExecute(2, numberOfLevels, cellsPerDimension);
  TestAmrDatasetExecute(3, numberOfLevels, cellsPerDimension);
}
} // namespace

int RegressionTestAmrDataset(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestAmrDataset, argc, argv);
}
