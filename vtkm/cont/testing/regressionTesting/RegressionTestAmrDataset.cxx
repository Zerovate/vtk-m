//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <fstream>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

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

  using M = vtkm::rendering::MapperRayTracer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;

  // Generate AMR
  vtkm::source::Amr source(dim, cellsPerDimension, numberOfLevels);
  vtkm::cont::AmrDataSet amrDataSet = source.Execute();
  //  std::cout << "amr " << std::endl;
  //  amrDataSet.PrintSummary(std::cout);

  // Remove blanked cells
  vtkm::filter::Threshold threshold;
  threshold.SetLowerThreshold(0);
  threshold.SetUpperThreshold(1);
  threshold.SetActiveField("vtkGhostType");
  vtkm::cont::PartitionedDataSet derivedDataSet = threshold.Execute(amrDataSet);
  //  std::cout << "derived " << std::endl;
  //  derivedDataSet.PrintSummary(std::cout);

  // Extract surface for efficient 3D pipeline
  vtkm::filter::ExternalFaces surface;
  surface.SetFieldsToPass("RTDataCells");
  derivedDataSet = surface.Execute(derivedDataSet);

  // Merge dataset
  vtkm::cont::DataSet result = vtkm::cont::MergePartitionedDataSet(derivedDataSet);
  //  std::cout << "merged " << std::endl;
  //  result.PrintSummary(std::cout);

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
