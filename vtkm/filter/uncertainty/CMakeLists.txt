##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(uncertainty_headers
  ContourUncertainUniform.h
  ContourUncertainUniformMonteCarlo.h

  #2 Variate Data
  2VariateUncertianty.h

  )
set(uncertainty_sources_device
  ContourUncertainUniform.cxx
  ContourUncertainUniformMonteCarlo.cxx

  #2 Variate
  2VariateUncertianty.cxx
  )

vtkm_library(
  NAME vtkm_filter_uncertainty
  HEADERS ${uncertainty_headers}
  DEVICE_SOURCES ${uncertainty_sources_device}
  USE_VTKM_JOB_POOL
)
