//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/uncertainty/StructuredReduceWithDistributionModel.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/cont/testing/Testing.h>

#include <random>

namespace
{

static constexpr vtkm::Float64 TOLERANCE = 0.0001;

#define VTKM_TEST_ASSERT_ARRAYS_EQUAL(array1, array2) \
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(array1, array2, TOLERANCE))

vtkm::cont::DataSet CreateData(vtkm::Id3 pointDimensions)
{
  vtkm::cont::DataSet dataSet;

  vtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(pointDimensions);
  dataSet.SetCellSet(cellSet);

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(pointDimensions);
  dataSet.AddCoordinateSystem({ "coordinates", coordinates });

  vtkm::cont::ArrayHandle<vtkm::Float32> pointScalars;
  pointScalars.Allocate(cellSet.GetNumberOfPoints());
  SetPortal(pointScalars.WritePortal());
  dataSet.AddPointField("point_scalars", pointScalars);

  vtkm::cont::ArrayHandle<vtkm::Float64> cellScalars;
  cellScalars.Allocate(cellSet.GetNumberOfCells());
  SetPortal(cellScalars.WritePortal());
  dataSet.AddCellField("cell_scalars", cellScalars);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> pointVectors;
  pointVectors.Allocate(cellSet.GetNumberOfPoints());
  SetPortal(pointVectors.WritePortal());
  dataSet.AddPointField("point_vectors", pointVectors);

  vtkm::cont::ArrayHandle<vtkm::Int32> intField;
  intField.Allocate(cellSet.GetNumberOfPoints());
  SetPortal(intField.WritePortal());
  dataSet.AddPointField("int_field", intField);

  return dataSet;
}

void TestUniformData()
{
  std::cout << "Testing uniform grid" << std::endl;

  vtkm::cont::DataSet input = CreateData({ 12, 8, 4 });

  vtkm::filter::uncertainty::StructuredReduceWithDistributionModel reduceFilter;
  reduceFilter.SetBlockSize({ 4, 4, 4 });
  reduceFilter.SetGenerateStandardDeviation(true);
  vtkm::cont::DataSet output = reduceFilter.Execute(input);

  vtkm::cont::CellSetStructured<3> cellSet;
  output.GetCellSet().AsCellSet(cellSet);
  VTKM_TEST_ASSERT(cellSet.GetPointDimensions() == vtkm::Id3{ 3, 2, 2 });
  VTKM_TEST_ASSERT(cellSet.GetCellDimensions() == vtkm::Id3{ 2, 1, 1 });

  vtkm::cont::UnknownArrayHandle fieldArray;

  fieldArray = output.GetCoordinateSystem().GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(fieldArray,
                                vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 1.5, 1.5, 0.5 },
                                                                            { 5.5, 1.5, 0.5 },
                                                                            { 9.5, 1.5, 0.5 },
                                                                            { 1.5, 5.5, 0.5 },
                                                                            { 5.5, 5.5, 0.5 },
                                                                            { 9.5, 5.5, 0.5 },
                                                                            { 1.5, 1.5, 2.5 },
                                                                            { 5.5, 1.5, 2.5 },
                                                                            { 9.5, 1.5, 2.5 },
                                                                            { 1.5, 5.5, 2.5 },
                                                                            { 5.5, 5.5, 2.5 },
                                                                            { 9.5, 5.5, 2.5 } }));

  fieldArray = output.GetPointField("coordinates_stddev").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(
    fieldArray,
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 },
                                                { 1.11803, 1.11803, 0.5 } }));

  fieldArray = output.GetPointField("point_scalars").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(
    fieldArray,
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 1.676, 1.716, 1.756, 2.156, 2.196, 2.236, 3.596, 3.636, 3.676, 4.076, 4.116, 4.156 }));

  fieldArray = output.GetPointField("point_scalars_stddev").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(fieldArray,
                                vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 0.498522,
                                                                              0.498523,
                                                                              0.498523,
                                                                              0.498522,
                                                                              0.498523,
                                                                              0.498522,
                                                                              0.498522,
                                                                              0.498524,
                                                                              0.498523,
                                                                              0.498521,
                                                                              0.498526,
                                                                              0.498525 }));

  fieldArray = output.GetCellField("cell_scalars").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(fieldArray,
                                vtkm::cont::make_ArrayHandle<vtkm::Float64>({ 1.566, 1.606 }));

  fieldArray = output.GetCellField("cell_scalars_stddev").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(fieldArray,
                                vtkm::cont::make_ArrayHandle<vtkm::Float64>({ 0.40432, 0.40432 }));

  fieldArray = output.GetPointField("point_vectors").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(
    fieldArray,
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 3.026, 3.036, 3.046 },
                                                { 3.146, 3.156, 3.166 },
                                                { 3.266, 3.276, 3.286 },
                                                { 4.466, 4.476, 4.486 },
                                                { 4.586, 4.596, 4.606 },
                                                { 4.706, 4.716, 4.726 },
                                                { 8.786, 8.796, 8.806 },
                                                { 8.906, 8.916, 8.926 },
                                                { 9.026, 9.036, 9.046 },
                                                { 10.226, 10.236, 10.246 },
                                                { 10.346, 10.356, 10.366 },
                                                { 10.466, 10.476, 10.486 } }));

  fieldArray = output.GetPointField("point_vectors_stddev").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(
    fieldArray,
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 1.49557, 1.49557, 1.49557 },
                                                { 1.49557, 1.49557, 1.49557 },
                                                { 1.49557, 1.49557, 1.49557 },
                                                { 1.49557, 1.49557, 1.49557 },
                                                { 1.49557, 1.49557, 1.49557 },
                                                { 1.49557, 1.49557, 1.49557 },
                                                { 1.49556, 1.49556, 1.49558 },
                                                { 1.49557, 1.49558, 1.49556 },
                                                { 1.49557, 1.49555, 1.49557 },
                                                { 1.49556, 1.49557, 1.49557 },
                                                { 1.49557, 1.49557, 1.49557 },
                                                { 1.49556, 1.49556, 1.49557 } }));

  fieldArray = output.GetPointField("int_field").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(
    fieldArray,
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 6750, 7150, 7550, 11550, 11950, 12350, 25950, 26350, 26750, 30750, 31150, 31550 }));

  fieldArray = output.GetPointField("int_field_stddev").GetData();
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(fieldArray,
                                vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 4985.23,
                                                                              4985.23,
                                                                              4985.23,
                                                                              4985.23,
                                                                              4985.23,
                                                                              4985.23,
                                                                              4985.23,
                                                                              4985.24,
                                                                              4985.24,
                                                                              4985.22,
                                                                              4985.24,
                                                                              4985.25 }));
}

void TestRenameSuffixes()
{
  std::cout << "Testing suffixes that are not the default name." << std::endl;

  vtkm::cont::DataSet input = CreateData({ 12, 8, 4 });

  vtkm::filter::uncertainty::StructuredReduceWithDistributionModel reduceFilter;
  reduceFilter.SetBlockSize({ 4, 4, 4 });
  reduceFilter.SetGenerateMean(true);
  reduceFilter.SetMeanSuffix("_avg");
  reduceFilter.SetGenerateStandardDeviation(true);
  reduceFilter.SetStandardDeviationSuffix("_sd");

  vtkm::cont::DataSet output = reduceFilter.Execute(input);

  for (vtkm::IdComponent fieldIndex = 0; fieldIndex < input.GetNumberOfFields(); ++fieldIndex)
  {
    vtkm::cont::Field inField = input.GetField(fieldIndex);
    VTKM_TEST_ASSERT(!output.HasField(inField.GetName(), inField.GetAssociation()));
    VTKM_TEST_ASSERT(output.HasField(inField.GetName() + "_avg", inField.GetAssociation()));
    VTKM_TEST_ASSERT(output.HasField(inField.GetName() + "_sd", inField.GetAssociation()));
  }

  VTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == input.GetNumberOfCoordinateSystems());
  for (vtkm::IdComponent csIndex = 0; csIndex < input.GetNumberOfCoordinateSystems(); ++csIndex)
  {
    std::string csName = input.GetCoordinateSystemName(csIndex);
    VTKM_TEST_ASSERT(output.HasCoordinateSystem(csName + "_avg"));
  }
}

void TryUniformPointCoordinates(const vtkm::Vec3f& origin, const vtkm::Vec3f& spacing)
{
  std::cout << "  trying origin=" << origin << "; spacing=" << spacing << std::endl;

  vtkm::IdComponent3 blockSize = { 4, 3, 2 };
  vtkm::Id3 pointSize = 2 * blockSize;
  vtkm::cont::DataSet input = CreateData(pointSize);

  vtkm::cont::ArrayHandleUniformPointCoordinates inputCoordArray{ pointSize, origin, spacing };
  input.AddCoordinateSystem({ "coordinates", inputCoordArray });

  vtkm::cont::ArrayHandle<vtkm::Vec3f> inputCoordCopy;
  vtkm::cont::ArrayCopy(inputCoordArray, inputCoordCopy);
  input.AddPointField("coord_copy", inputCoordCopy);

  vtkm::filter::uncertainty::StructuredReduceWithDistributionModel reduceFilter;
  reduceFilter.SetBlockSize(blockSize);
  vtkm::cont::DataSet output = reduceFilter.Execute(input);

  vtkm::cont::UnknownArrayHandle uniformOutput;
  vtkm::cont::UnknownArrayHandle basicOutput;

  uniformOutput = output.GetCoordinateSystem().GetData();
  basicOutput = output.GetPointField("coord_copy").GetData();
  VTKM_TEST_ASSERT(uniformOutput.CanConvert<vtkm::cont::ArrayHandleUniformPointCoordinates>());
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(uniformOutput, basicOutput);

  uniformOutput = output.GetPointField("coordinates_stddev").GetData();
  basicOutput = output.GetPointField("coord_copy_stddev").GetData();
  VTKM_TEST_ASSERT(uniformOutput.CanConvert<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f>>());
  VTKM_TEST_ASSERT_ARRAYS_EQUAL(uniformOutput, basicOutput);
}

void TestUniformPointCoordinates()
{
  std::cout << "Testing handling of uniform point coordinates." << std::endl;

  std::random_device::result_type seed;
  seed = std::random_device{}();
  std::cout << "  seed = " << seed << std::endl;
  std::mt19937 generator{ seed };
  std::uniform_real_distribution<vtkm::FloatDefault> distribution{ -2.0, 2.0 };
  auto myrand = [&]() { return distribution(generator); };

  TryUniformPointCoordinates({ 0, 0, 0 }, { 1, 1, 1 });
  TryUniformPointCoordinates({ myrand(), myrand(), myrand() }, { 1, 1, 1 });
  TryUniformPointCoordinates({ 0, 0, 0 }, { myrand(), myrand(), myrand() });
  TryUniformPointCoordinates({ myrand(), myrand(), myrand() }, { myrand(), myrand(), myrand() });
}

void DoTest()
{
  TestUniformData();
  TestRenameSuffixes();
  TestUniformPointCoordinates();
}

} // anonymous namespace

int UnitTestStructuredReduceWithDistributionModel(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
