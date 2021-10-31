##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(unit_tests
  UnitTestContourFilter.cxx
  UnitTestContourFilterNormals.cxx
)

set(libraries
  vtkm_filter
  vtkm_io
  vtkm_source
)

if (VTKm_ENABLE_RENDERING)
  list(APPEND libraries vtkm_rendering)

  list(APPEND unit_tests
    RegressionTestContourFilter.cxx
    RegressionTestSliceFilter.cxx
  )
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  ALL_BACKENDS
  USE_VTKM_JOB_POOL
)
