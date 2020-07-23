//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleRandomBernoulli.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DescriptiveStatistics.h>

void TestArrayHandleBernoulliForP(vtkm::Float32 p)
{
  auto randomArray =
    vtkm::cont::ArrayHandleRandomBernoulli<vtkm::Float32, vtkm::Float32>(1000000, p, { 0xceed });
  auto stats = vtkm::worklet::DescriptiveStatistics::Run(randomArray);

  vtkm::Float32 q = 1 - p;
  VTKM_TEST_ASSERT(test_equal(stats.Mean(), p, 1.0f / 100));
  VTKM_TEST_ASSERT(test_equal(stats.PopulationVariance(), p * q, 1.0f / 100));
  if (p > 0.001f && q > 0.001f)
  {
    VTKM_TEST_ASSERT(test_equal(stats.Skewness(), (q - p) / vtkm::Sqrt(p * q), 1.0f / 100));
    float excess_kurtosis = (6 * p * p - 6 * p + 1) / (p * q);
    VTKM_TEST_ASSERT(test_equal(stats.Kurtosis(), excess_kurtosis + 3, 1.0f / 10));
  }
}

void TestArrayHandleBernoulli()
{
  TestArrayHandleBernoulliForP(0.0f);
  TestArrayHandleBernoulliForP(0.1f);
  TestArrayHandleBernoulliForP(0.25f);
  TestArrayHandleBernoulliForP(0.5f);
  TestArrayHandleBernoulliForP(0.75f);
  TestArrayHandleBernoulliForP(0.9f);
  TestArrayHandleBernoulliForP(1.0f);
}

int UnitTestArrayHandleRandomBernoulli(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleBernoulli, argc, argv);
}
