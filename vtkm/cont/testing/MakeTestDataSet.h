//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_testing_MakeTestDataSet_h
#define vtk_m_cont_testing_MakeTestDataSet_h

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>

#include <vtkm/filter/VectorMagnitude.h>

#include <vtkm/Deprecated.h>

#include <vtkm/cont/testing/Testing.h>

#include <numeric>

namespace vtkm
{
namespace cont
{
namespace testing
{

VTKM_DEPRECATED(1.6,
                "Don't include MakeTestDataSet.h. Instead, load test files with "
                "vtkm::cont::testing::Testing::ReadVTKFile().")
inline void MakeTestDataSet_h_deprecated() {}

inline void ActivateMakeTestDataSet_h_deprecated_warning()
{
  MakeTestDataSet_h_deprecated();
}

class MakeTestDataSet
{
public:
  // 1D uniform datasets.
  vtkm::cont::DataSet Make1DUniformDataSet0();
  vtkm::cont::DataSet Make1DUniformDataSet1();
  vtkm::cont::DataSet Make1DUniformDataSet2();

  // 1D explicit datasets.
  vtkm::cont::DataSet Make1DExplicitDataSet0();

  // 2D uniform datasets.
  vtkm::cont::DataSet Make2DUniformDataSet0();
  vtkm::cont::DataSet Make2DUniformDataSet1();
  vtkm::cont::DataSet Make2DUniformDataSet2();
  vtkm::cont::DataSet Make2DUniformDataSet3();

  // 3D uniform datasets.
  vtkm::cont::DataSet Make3DUniformDataSet0();
  vtkm::cont::DataSet Make3DUniformDataSet1();
  vtkm::cont::DataSet Make3DUniformDataSet2();
  vtkm::cont::DataSet Make3DUniformDataSet3(const vtkm::Id3 dims = vtkm::Id3(10));
  vtkm::cont::DataSet Make3DUniformDataSet4();
  vtkm::cont::DataSet Make3DRegularDataSet0();
  vtkm::cont::DataSet Make3DRegularDataSet1();

  //2D rectilinear
  vtkm::cont::DataSet Make2DRectilinearDataSet0();

  //3D rectilinear
  vtkm::cont::DataSet Make3DRectilinearDataSet0();

  // 2D explicit datasets.
  vtkm::cont::DataSet Make2DExplicitDataSet0();

  // 3D explicit datasets.
  vtkm::cont::DataSet Make3DExplicitDataSet0();
  vtkm::cont::DataSet Make3DExplicitDataSet1();
  vtkm::cont::DataSet Make3DExplicitDataSet2();
  vtkm::cont::DataSet Make3DExplicitDataSet3();
  vtkm::cont::DataSet Make3DExplicitDataSet4();
  vtkm::cont::DataSet Make3DExplicitDataSet5();
  vtkm::cont::DataSet Make3DExplicitDataSet6();
  vtkm::cont::DataSet Make3DExplicitDataSet7();
  vtkm::cont::DataSet Make3DExplicitDataSet8();
  vtkm::cont::DataSet Make3DExplicitDataSetZoo();
  vtkm::cont::DataSet Make3DExplicitDataSetPolygonal();
  vtkm::cont::DataSet Make3DExplicitDataSetCowNose();
};

//Make a simple 1D dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet1D_0.vtk");
}

//Make another simple 1D dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet1()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet1D_1.vtk");
}


//Make a simple 1D, 16 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet2()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet1D_2.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make1DExplicitDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet1D_0.vtk");
}

//Make a simple 2D, 2 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet2D_0.vtk");
}

//Make a simple 2D, 16 cell uniform dataset (5x5.txt)
inline vtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet1()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet2D_1.vtk");
}

//Make a simple 2D, 16 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet2()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet2D_2.vtk");
}

//Make a simple 2D, 56 cell uniform dataset. (8x9test.txt)
inline vtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet3()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet2D_3.vtk");
}

//Make a simple 3D, 4 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_0.vtk");
}

//Make a simple 3D, 64 cell uniform dataset. (5b 5x5x5)
inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet1()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_1.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet2()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::filter::VectorMagnitude magnitude;
  magnitude.SetUseCoordinateSystemAsField(true);
  magnitude.SetOutputFieldName("pointvar");
  return magnitude.Execute(dsb.Create(vtkm::Id3(256)));
  //  constexpr vtkm::Id base_size = 256;
  //  vtkm::cont::DataSetBuilderUniform dsb;
  //  vtkm::Id3 dimensions(base_size, base_size, base_size);
  //  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  //  constexpr vtkm::Id nVerts = base_size * base_size * base_size;
  //  vtkm::Float32* pointvar = new vtkm::Float32[nVerts];

  //  for (vtkm::Id z = 0; z < base_size; ++z)
  //    for (vtkm::Id y = 0; y < base_size; ++y)
  //      for (vtkm::Id x = 0; x < base_size; ++x)
  //      {
  //        std::size_t index = static_cast<std::size_t>(z * base_size * base_size + y * base_size + x);
  //        pointvar[index] = vtkm::Sqrt(vtkm::Float32(x * x + y * y + z * z));
  //      }

  //  dataSet.AddPointField("pointvar", pointvar, nVerts);

  //  delete[] pointvar;

  //  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet3(const vtkm::Id3 dims)
{
  // Use wavelet or tangle source
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::cont::DataSet dataSet = dsb.Create(dims);

  // add point scalar field
  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];
  std::vector<vtkm::Float64> pointvar(static_cast<size_t>(numPoints));

  vtkm::Float64 dx = vtkm::Float64(4.0 * vtkm::Pi()) / vtkm::Float64(dims[0] - 1);
  vtkm::Float64 dy = vtkm::Float64(2.0 * vtkm::Pi()) / vtkm::Float64(dims[1] - 1);
  vtkm::Float64 dz = vtkm::Float64(3.0 * vtkm::Pi()) / vtkm::Float64(dims[2] - 1);

  vtkm::Id idx = 0;
  for (vtkm::Id z = 0; z < dims[2]; ++z)
  {
    vtkm::Float64 cz = vtkm::Float64(z) * dz - 1.5 * vtkm::Pi();
    for (vtkm::Id y = 0; y < dims[1]; ++y)
    {
      vtkm::Float64 cy = vtkm::Float64(y) * dy - vtkm::Pi();
      for (vtkm::Id x = 0; x < dims[0]; ++x)
      {
        vtkm::Float64 cx = vtkm::Float64(x) * dx - 2.0 * vtkm::Pi();
        vtkm::Float64 cv = vtkm::Sin(cx) + vtkm::Sin(cy) +
          2.0 * vtkm::Cos(vtkm::Sqrt((cx * cx) / 2.0 + cy * cy) / 0.75) +
          4.0 * vtkm::Cos(cx * cy / 4.0);

        if (dims[2] > 1)
        {
          cv += vtkm::Sin(cz) + 1.5 * vtkm::Cos(vtkm::Sqrt(cx * cx + cy * cy + cz * cz) / 0.75);
        }
        pointvar[static_cast<size_t>(idx)] = cv;
        idx++;
      }
    } // y
  }   // z

  dataSet.AddPointField("pointvar", pointvar);

  vtkm::Id numCells = (dims[0] - 1) * (dims[1] - 1) * (dims[2] - 1);
  vtkm::cont::ArrayHandle<vtkm::Float64> cellvar;
  vtkm::cont::ArrayCopy(
    vtkm::cont::make_ArrayHandleCounting(vtkm::Float64(0), vtkm::Float64(1), numCells), cellvar);
  dataSet.AddCellField("cellvar", cellvar);

  return dataSet;
}

//Make a simple 3D, 120 cell uniform dataset. (This is the data set from
//Make3DUniformDataSet1 upsampled from 5x5x to 5x6x7.)
inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet4()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_4.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make2DRectilinearDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("rectilinear/RectilinearDataSet2D_0.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DRegularDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_2.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DRegularDataSet1()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("uniform/UniformDataSet3D_3.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DRectilinearDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("rectilinear/RectilinearDataSet3D_0.vtk");
}

// Make a 2D explicit dataset
inline vtkm::cont::DataSet MakeTestDataSet::Make2DExplicitDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet2D_0.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet0()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_0.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet1()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_1.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet2()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_2.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet4()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_4.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet3()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_3.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet5()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_5.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet6()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_6.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetZoo()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_Zoo.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet7()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_7.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet8()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_8.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetPolygonal()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_Polygonal.vtk");
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetCowNose()
{
  return vtkm::cont::testing::Testing::ReadVTKFile("unstructured/ExplicitDataSet3D_CowNose.vtk");
}
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_MakeTestDataSet_h
