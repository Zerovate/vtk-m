//
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <vtkm/Math.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/UnknownArrayHandle.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/uncertainty/2VariateUncertianty.h>
#include <vtkm/filter/uncertainty/ContourUncertainUniform.h>
#include <vtkm/filter/uncertainty/ContourUncertainUniformMonteCarlo.h>


vtkm::cont::DataSet Make2VariateDataSet()
{
  const vtkm::Id3 dims(20, 20, 20);
  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];
  vtkm::cont::DataSetBuilderUniform dsBuilder;
  vtkm::cont::DataSet ds = dsBuilder.Create(dims);


  std::vector<vtkm::FloatDefault> ensembleMinX(numPoints, 10);
  std::vector<vtkm::FloatDefault> ensembleMaxX(numPoints, 20);
  std::vector<vtkm::FloatDefault> ensembleMinY(numPoints, 10);
  std::vector<vtkm::FloatDefault> ensembleMaxY(numPoints, 20);

  ds.AddPointField("ensemble_min_x", ensembleMinX);
  ds.AddPointField("ensemble_max_x", ensembleMaxX);
  ds.AddPointField("ensemble_min_y", ensembleMinY);
  ds.AddPointField("ensemble_max_y", ensembleMaxY);

  return ds;
}

void Test2VariateUncertaintyComparison()
{
 .
  vtkm::cont::DataSet ds = Make2VariateDataSet();


  vtkm::Pair<vtkm::FloatDefault, vtkm::FloatDefault> minAxis(10.0, 10.0);
  vtkm::Pair<vtkm::FloatDefault, vtkm::FloatDefault> maxAxis(20.0, 20.0);


  const vtkm::FloatDefault delta = 0.1f;


  vtkm::filter::uncertainty::FiberMean closedFormFilter;
  closedFormFilter.SetMinAxis(minAxis);
  closedFormFilter.SetMaxAxis(maxAxis);
  closedFormFilter.SetMinX("ensemble_min_x");
  closedFormFilter.SetMaxX("ensemble_max_x");
  closedFormFilter.SetMinY("ensemble_min_y");
  closedFormFilter.SetMaxY("ensemble_max_y");
  closedFormFilter.SetApproach("ClosedForm");

  vtkm::cont::DataSet outputClosed = closedFormFilter.Execute(ds);
  vtkm::cont::Field closedField = outputClosed.GetField("ClosedForm");
  vtkm::cont::UnknownArrayHandle unknownClosed = closedField.GetData();

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> closedArray;
  unknownClosed.AsArrayHandle(closedArray);
  auto closedPortal = closedArray.ReadPortal();


  vtkm::filter::uncertainty::FiberMean monteCarloFilter;
  monteCarloFilter.SetMinAxis(minAxis);
  monteCarloFilter.SetMaxAxis(maxAxis);
  monteCarloFilter.SetMinX("ensemble_min_x");
  monteCarloFilter.SetMaxX("ensemble_max_x");
  monteCarloFilter.SetMinY("ensemble_min_y");
  monteCarloFilter.SetMaxY("ensemble_max_y");
  monteCarloFilter.SetApproach("MonteCarlo");

  monteCarloFilter.SetNumSamples(1000);
  vtkm::cont::DataSet outputMC = monteCarloFilter.Execute(ds);
  vtkm::cont::Field monteField = outputMC.GetField("MonteCarlo");
  vtkm::cont::UnknownArrayHandle unknownMC = monteField.GetData();
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> monteArray;
  unknownMC.AsArrayHandle(monteArray);
  auto montePortal = monteArray.ReadPortal();
  vtkm::Id numValues = closedArray.GetNumberOfValues();
  std::cout << "Comparing outputs for " << numValues << " values." << std::endl;
  for (vtkm::Id i = 0; i < numValues; ++i)
  {
    vtkm::FloatDefault diff = std::fabs(closedPortal.Get(i) - montePortal.Get(i));
    VTKM_TEST_ASSERT(diff <= delta, "Difference between ClosedForm and MonteCarlo value too large.");
  }
}
int UnitTest2VariateUncertaintyComparison(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test2VariateUncertaintyComparison, argc, argv);
}




int UnitTest2VariateUncertainty(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test2VariateUncertainty, argc, argv);
}
