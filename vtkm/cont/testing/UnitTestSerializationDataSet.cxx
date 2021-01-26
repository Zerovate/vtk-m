//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/testing/TestingSerialization.h>

using namespace vtkm::cont::testing::serialization;

namespace
{

using FieldTypeList = vtkm::List<vtkm::Float32, vtkm::Float64, vtkm::Vec3f>;
using CellSetTypes = vtkm::List<vtkm::cont::CellSetExplicit<>,
                                vtkm::cont::CellSetSingleType<>,
                                vtkm::cont::CellSetStructured<1>,
                                vtkm::cont::CellSetStructured<2>,
                                vtkm::cont::CellSetStructured<3>>;

using DataSetWrapper = vtkm::cont::SerializableDataSet<FieldTypeList, CellSetTypes>;

VTKM_CONT void TestEqualDataSet(const DataSetWrapper& ds1, const DataSetWrapper& ds2)
{
  VTKM_TEST_ASSERT(test_equal_DataSets(ds1.DataSet, ds2.DataSet, CellSetTypes{}));
}

void RunTest(const std::string& file)
{
  std::cout << "Testing " << file << "\n";
  vtkm::cont::DataSet ds = vtkm::cont::testing::Testing::ReadVTKFile(file);
  TestSerialization(DataSetWrapper(ds), TestEqualDataSet);
}

void TestDataSetSerialization()
{
  RunTest("uniform/UniformDataSet1D_0.vtk");
  RunTest("uniform/UniformDataSet1D_1.vtk");

  RunTest("uniform/UniformDataSet2D_0.vtk");
  RunTest("uniform/UniformDataSet2D_1.vtk");

  RunTest("uniform/UniformDataSet3D_0.vtk");
  RunTest("uniform/UniformDataSet3D_1.vtk");
  RunTest("uniform/UniformDataSet3D_2.vtk");

  RunTest("rectilinear/RectilinearDataSet2D_0.vtk");
  RunTest("rectilinear/RectilinearDataSet3D_0.vtk");

  RunTest("unstructured/ExplicitDataSet1D_0.vtk");

  RunTest("unstructured/ExplicitDataSet2D_0.vtk");

  RunTest("unstructured/ExplicitDataSet3D_0.vtk");
  RunTest("unstructured/ExplicitDataSet3D_1.vtk");
  RunTest("unstructured/ExplicitDataSet3D_2.vtk");
  RunTest("unstructured/ExplicitDataSet3D_3.vtk");
  RunTest("unstructured/ExplicitDataSet3D_4.vtk");
  RunTest("unstructured/ExplicitDataSet3D_5.vtk");
  RunTest("unstructured/ExplicitDataSet3D_6.vtk");

  RunTest("unstructured/ExplicitDataSet3D_Polygonal.vtk");

  RunTest("unstructured/ExplicitDataSet3D_CowNose.vtk");
}

} // anonymous namespace

int UnitTestSerializationDataSet(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSetSerialization, argc, argv);
}
