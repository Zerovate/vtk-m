##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(imageprocessing_headers
  ImageDifference.h)

set(imageprocessing_sources_device
  ImageDifference.cxx)

vtkm_library(
  NAME vtkm_filter_imageprocessing
  HEADERS ${imageprocessing_headers}
  DEVICE_SOURCES ${imageprocessing_sources_device}
  USE_VTKM_JOB_POOL
)

set_property(TARGET
  vtkm_filter_imageprocessing
  PROPERTY UNITY_BUILD_MODE GROUP
  )

target_link_libraries(vtkm_filter_imageprocessing PUBLIC vtkm_worklet)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_imageprocessing)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
