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
#include <complex>
#include <cstdio>
#include <vector>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/cont/DataSetBuilderUniform.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

#define WRITE_FILE(MakeTestDataMethod) \
  TestVTKWriteTestData(#MakeTestDataMethod, tds.MakeTestDataMethod())

struct CheckSameField
{
  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& originalArray,
                  const vtkm::cont::Field& fileField) const
  {
    vtkm::cont::ArrayHandle<T> fileArray;
    fileField.GetData().AsArrayHandle(fileArray);
    VTKM_TEST_ASSERT(test_equal_portals(originalArray.ReadPortal(), fileArray.ReadPortal()));
  }
};

struct CheckSameCoordinateSystem
{
  template <typename T>
  void operator()(const vtkm::cont::ArrayHandle<T>& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    CheckSameField{}(originalArray, fileCoords);
  }

  template <typename T>
  void operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagSOA>& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    CheckSameField{}(originalArray, fileCoords);
  }

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  template <typename T>
  void operator()(const vtkm::cont::ArrayHandleVirtual<T>& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    CheckSameField{}(originalArray, fileCoords);
  }
  VTKM_DEPRECATED_SUPPRESS_END
#endif

  void operator()(const vtkm::cont::ArrayHandleUniformPointCoordinates& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    VTKM_TEST_ASSERT(fileCoords.GetData().IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>());
    vtkm::cont::ArrayHandleUniformPointCoordinates fileArray =
      fileCoords.GetData().AsArrayHandle<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    auto originalPortal = originalArray.ReadPortal();
    auto filePortal = fileArray.ReadPortal();
    VTKM_TEST_ASSERT(test_equal(originalPortal.GetOrigin(), filePortal.GetOrigin()));
    VTKM_TEST_ASSERT(test_equal(originalPortal.GetSpacing(), filePortal.GetSpacing()));
    VTKM_TEST_ASSERT(test_equal(originalPortal.GetRange3(), filePortal.GetRange3()));
  }

  template <typename T>
  using ArrayHandleRectilinearCoords = vtkm::cont::ArrayHandle<
    T,
    typename vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T>,
                                                     vtkm::cont::ArrayHandle<T>,
                                                     vtkm::cont::ArrayHandle<T>>::StorageTag>;
  template <typename T>
  void operator()(const ArrayHandleRectilinearCoords<T>& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    VTKM_TEST_ASSERT(fileCoords.GetData().IsType<ArrayHandleRectilinearCoords<T>>());
    ArrayHandleRectilinearCoords<T> fileArray =
      fileCoords.GetData().AsArrayHandle<ArrayHandleRectilinearCoords<T>>();
    auto originalPortal = originalArray.ReadPortal();
    auto filePortal = fileArray.ReadPortal();
    VTKM_TEST_ASSERT(
      test_equal_portals(originalPortal.GetFirstPortal(), filePortal.GetFirstPortal()));
    VTKM_TEST_ASSERT(
      test_equal_portals(originalPortal.GetSecondPortal(), filePortal.GetSecondPortal()));
    VTKM_TEST_ASSERT(
      test_equal_portals(originalPortal.GetThirdPortal(), filePortal.GetThirdPortal()));
  }
};

void CheckWrittenReadData(const vtkm::cont::DataSet& originalData,
                          const vtkm::cont::DataSet& fileData)
{
  VTKM_TEST_ASSERT(originalData.GetNumberOfPoints() == fileData.GetNumberOfPoints());
  VTKM_TEST_ASSERT(originalData.GetNumberOfCells() == fileData.GetNumberOfCells());

  for (vtkm::IdComponent fieldId = 0; fieldId < originalData.GetNumberOfFields(); ++fieldId)
  {
    vtkm::cont::Field originalField = originalData.GetField(fieldId);
    VTKM_TEST_ASSERT(fileData.HasField(originalField.GetName(), originalField.GetAssociation()));
    vtkm::cont::Field fileField =
      fileData.GetField(originalField.GetName(), originalField.GetAssociation());
    vtkm::cont::CastAndCall(originalField, CheckSameField{}, fileField);
  }

  VTKM_TEST_ASSERT(fileData.GetNumberOfCoordinateSystems() > 0);
  vtkm::cont::CastAndCall(originalData.GetCoordinateSystem().GetData(),
                          CheckSameCoordinateSystem{},
                          fileData.GetCoordinateSystem());
}

void TestVTKWriteTestData(const std::string& inputfile)
{
  std::cout << "Writing " << inputfile << std::endl;
  vtkm::cont::DataSet data = vtkm::cont::testing::Testing::ReadVTKFile(inputfile);

  vtkm::io::VTKDataSetWriter writer("testwrite.vtk");
  writer.WriteDataSet(data);

  // Read back and check.
  vtkm::io::VTKDataSetReader reader("testwrite.vtk");
  CheckWrittenReadData(data, reader.ReadDataSet());
}

void TestVTKExplicitWrite()
{
  TestVTKWriteTestData("unstructured/ExplicitDataSet1D_0.vtk");
  TestVTKWriteTestData("unstructured/ExplicitDataSet2D_0.vtk");
  TestVTKWriteTestData("unstructured/ExplicitDataSet3D_CowNose.vtk");
  TestVTKWriteTestData("unstructured/ExplicitDataSet3D_Polygonal.vtk");
  TestVTKWriteTestData("unstructured/ExplicitDataSet3D_Zoo.vtk");
}

void TestVTKUniformWrite()
{
  TestVTKWriteTestData("uniform/UniformDataSet1D_0.vtk");

  TestVTKWriteTestData("uniform/UniformDataSet2D_0.vtk");
  TestVTKWriteTestData("uniform/UniformDataSet2D_1.vtk");

  TestVTKWriteTestData("uniform/UniformDataSet3D_0.vtk");
  TestVTKWriteTestData("uniform/UniformDataSet3D_1.vtk");
}

void TestVTKRectilinearWrite()
{
  TestVTKWriteTestData("rectilinear/RectilinearDataSet2D_0.vtk");

  TestVTKWriteTestData("rectilinear/RectilinearDataSet3D_0.vtk");
}

void TestVTKCompoundWrite()
{
  double s_min = 0.00001;
  double s_max = 1.0;
  double t_min = -2.0;
  double t_max = 2.0;
  int s_samples = 16;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dims(s_samples, s_samples);
  vtkm::Vec2f_64 origin(t_min, s_min);
  vtkm::Float64 ds = (s_max - s_min) / vtkm::Float64(dims[0] - 1);
  vtkm::Float64 dt = (t_max - t_min) / vtkm::Float64(dims[1] - 1);
  vtkm::Vec2f_64 spacing(dt, ds);
  vtkm::cont::DataSet dataSet = dsb.Create(dims, origin, spacing);
  size_t nVerts = static_cast<size_t>(s_samples * s_samples);
  std::vector<vtkm::Vec2f_64> points(nVerts);

  size_t idx = 0;
  for (vtkm::Id y = 0; y < dims[0]; ++y)
  {
    for (vtkm::Id x = 0; x < dims[1]; ++x)
    {
      double s = s_min + static_cast<vtkm::Float64>(y) * ds;
      double t = t_min + static_cast<vtkm::Float64>(x) * dt;
      // This function is not meaningful:
      auto z = std::exp(std::complex<double>(s, t));
      points[idx] = { std::sqrt(std::norm(z)), std::arg(z) };
      idx++;
    }
  }

  dataSet.AddPointField("z", points.data(), static_cast<vtkm::Id>(points.size()));
  vtkm::io::VTKDataSetWriter writer("chirp.vtk");
  writer.WriteDataSet(dataSet);
  std::remove("chirp.vtk");
}

void TestVTKWrite()
{
  TestVTKExplicitWrite();
  TestVTKUniformWrite();
  TestVTKRectilinearWrite();
  TestVTKCompoundWrite();
}

} //Anonymous namespace

int UnitTestVTKDataSetWriter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVTKWrite, argc, argv);
}
