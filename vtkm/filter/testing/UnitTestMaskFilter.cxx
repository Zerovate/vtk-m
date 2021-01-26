//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/Mask.h>

namespace
{

class TestingMask
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing mask cells uniform grid :" << std::endl;
    vtkm::cont::DataSet dataset =
      vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet2D_1.vtk");

    // Setup and run filter to extract by stride
    vtkm::filter::Mask mask;
    vtkm::Id stride = 2;
    mask.SetStride(stride);

    vtkm::cont::DataSet output = mask.Execute(dataset);

    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8), "Wrong result for Mask");


    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.ReadPortal().Get(7) == 14.f,
                     "Wrong mask data");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing mask cells uniform grid :" << std::endl;
    vtkm::cont::DataSet dataset =
      vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_1.vtk");

    // Setup and run filter to extract by stride
    vtkm::filter::Mask mask;
    vtkm::Id stride = 9;
    mask.SetStride(stride);

    vtkm::cont::DataSet output = mask.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 7), "Wrong result for Mask");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 7 &&
                       cellFieldArray.ReadPortal().Get(2) == 18.f,
                     "Wrong mask data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing mask cells explicit:" << std::endl;
    vtkm::cont::DataSet dataset =
      vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_5.vtk");

    // Setup and run filter to extract by stride
    vtkm::filter::Mask mask;
    vtkm::Id stride = 2;
    mask.SetStride(stride);

    vtkm::cont::DataSet output = mask.Execute(dataset);
    VTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 2), "Wrong result for Mask");

    vtkm::cont::ArrayHandle<vtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.ReadPortal().Get(1) == 120.2f,
                     "Wrong mask data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit();
  }
};
}

int UnitTestMaskFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingMask(), argc, argv);
}
