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

void display_param();

void read_params(int argc, char** argv, Parameters& params);

#endif
