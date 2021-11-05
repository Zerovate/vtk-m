##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(densityestimate_headers
  Histogram.h)
set(densityestimate_sources_device
  Histogram.cxx)

vtkm_library(
  NAME vtkm_filter_densityestimate
  HEADERS ${densityestimate_headers}
  DEVICE_SOURCES ${densityestimate_sources_device}
  USE_VTKM_JOB_POOL
)

set_property(TARGET
  vtkm_filter_densityestimate
  PROPERTY UNITY_BUILD_MODE GROUP
  )

target_link_libraries(vtkm_filter_densityestimate PUBLIC vtkm_worklet vtkm_filter_common)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_densityestimate)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
