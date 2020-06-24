//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <iomanip>
#include <utility>

struct Parameters
{
  std::string filename;
  std::tuple<float, float> temperature = std::make_tuple(100.f, 10.f);
  int dimension = 2000;
  int iteration = 1000;
  float diffuse_coeff = 0.6f;
  bool rendering_enable = true;
  bool create_matrix = true;
  bool twoD = false;
};

void display_param()
{
  std::cout << "\n\nParameters: " << std::endl;
  std::cout << "\t\t-h or --help\t\t\t\t\t\tHelp\n" << std::endl;
  std::cout << "\t\t-d [DEVICE]\t\t\tAny, Serial, OpenMP, TBB or Cuda\n" << std::endl;
  std::cout << "\t\t-f [FILENAME]\t\t\tName of the file you want to treat\n" << std::endl;
  std::cout << "\t\t-p \t\t\t\tEnable performance testing\n" << std::endl;
  std::cout << "\t\t-t [TEMP_OUTSIDE] [TEMP_INSIDE]\tChange the temperature of the "
               "dataset.\n\t\t\t\t\t\t\t\tDefault tempratures are 100 and 10\n"
            << std::endl;
  std::cout << "\t\t-s [DIMENSION]\t\t\tChange the size of the dataset.\n\t\t\t\t\t\t\t\tDefault "
               "size is 2000*2000\n"
            << std::endl;
  std::cout << "\t\t-i [NB_ITERATION]\t\tChange the number of iteration for the "
               "filter.\n\t\t\t\t\t\t\t\tDefault number of iteration is 100\n"
            << std::endl;
  std::cout << "\t\t-c [DIFF_COEFF]\t\t\tChange the diffusion coefficient of the dataset "
               ".\n\t\t\t\t\t\t\t\tDefault coefficient of diffusion is 0.6\n"
            << std::endl;
  std::cout << "\t\t-2d\t\t\t\t 2D rendering, default is 3D" << std::endl;
}

void read_params(int argc, char** argv, Parameters& params)
{
  int i = 0;
  int tmpI;

  while (i < argc - 1)
  {
    ++i;
    if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i]))
    {
      display_param();
      exit(EXIT_FAILURE);
      continue;
    }
    if (!strcmp("-p", argv[i]))
    {
      params.rendering_enable = false;
      continue;
    }
    if (!strcmp("-t", argv[i]))
    {
      std::get<0>(params.temperature) = strtof(argv[++i], nullptr);
      std::get<1>(params.temperature) = strtof(argv[++i], nullptr);
      continue;
    }
    if (!strcmp("-s", argv[i]))
    {
      tmpI = strtol(argv[i + 1], nullptr, 10);
      if (tmpI > 9)
      {
        params.dimension = tmpI;
        i++;
      }
      else
      {
        display_param();
        exit(EXIT_FAILURE);
      }
    }
    if (!strcmp("-c", argv[i]))
    {
      params.diffuse_coeff = strtof(argv[++i], nullptr);
    }
    if (!strcmp("-i", argv[i]))
    {
      params.iteration = strtol(argv[++i], nullptr, 10);
    }
    if (!strcmp("-2d", argv[i]))
    {
      params.twoD = true;
    }
  }
}

#endif
