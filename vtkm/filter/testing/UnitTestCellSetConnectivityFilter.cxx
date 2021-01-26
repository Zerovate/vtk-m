//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CellSetConnectivity.h>
#include <vtkm/filter/Contour.h>
#include <vtkm/source/Tangle.h>

namespace
{

class TestCellSetConnectivity
{
public:
  void TestTangleIsosurface() const
  {
    vtkm::Id3 dims(4, 4, 4);
    vtkm::source::Tangle tangle(dims);
    vtkm::cont::DataSet dataSet = tangle.Execute();

    vtkm::filter::Contour filter;
    filter.SetGenerateNormals(true);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(0, 0.1);
    filter.SetActiveField("nodevar");
    vtkm::cont::DataSet iso = filter.Execute(dataSet);

    vtkm::filter::CellSetConnectivity connectivity;
    const vtkm::cont::DataSet output = connectivity.Execute(iso);

    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 8,
                     "Wrong number of connected components");
  }

  void TestExplicitDataSet() const
  {
    vtkm::cont::DataSet dataSet =
      vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_5.vtk");

    vtkm::filter::CellSetConnectivity connectivity;
    const vtkm::cont::DataSet output = connectivity.Execute(dataSet);

    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void TestUniformDataSet() const
  {
    vtkm::cont::DataSet dataSet =
      vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_1.vtk");
    vtkm::filter::CellSetConnectivity connectivity;
    const vtkm::cont::DataSet output = connectivity.Execute(dataSet);

    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void operator()() const
  {
    this->TestTangleIsosurface();
    this->TestExplicitDataSet();
    this->TestUniformDataSet();
  }
};
}

int UnitTestCellSetConnectivityFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetConnectivity(), argc, argv);
}
