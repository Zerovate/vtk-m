//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Initialize.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/filter/ClipWithField.h>

int main(int argc, char* argv[])
{
  vtkm::cont::InitializeResult initResult = vtkm::cont::Initialize(argc, argv);

  if (argc != 2)
  {
    std::cerr << "USAGE: " << argv[0] << " [options] <vtk-file>\n";
    std::cerr << "options are:\n";
    std::cerr << initResult.Usage << "\n";
    std::cerr << "For the input file, consider "
                 "vtk-m/data/data/unstructured/ExplicitDataSet3D_CowNose.vtk\n";
    return 1;
  }

  vtkm::io::VTKDataSetReader reader(argv[1]);
  vtkm::cont::DataSet input = reader.ReadDataSet();

  vtkm::filter::ClipWithField clipFilter;
  clipFilter.SetActiveField("pointvar");
  clipFilter.SetClipValue(20.0);
  vtkm::cont::DataSet output = clipFilter.Execute(input);

  vtkm::io::VTKDataSetWriter writer("out_data.vtk");
  writer.WriteDataSet(output);

  return 0;
}
