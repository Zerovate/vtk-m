##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(entityextraction_headers
  ExternalFaces.h)

set(entityextraction_sources_device
  ExternalFaces.cxx)

vtkm_library(
  NAME vtkm_filter_entityextraction
  HEADERS ${entityextraction_headers}
  DEVICE_SOURCES ${entityextraction_sources_device}
  USE_VTKM_JOB_POOL
)

set_property(TARGET
  vtkm_filter_entityextraction
  PROPERTY UNITY_BUILD_MODE GROUP
)

target_link_libraries(vtkm_filter_entityextraction PUBLIC vtkm_worklet vtkm_filter_common vtkm_filter_cleangrid)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_entityextraction)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
