##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(vectorcalculus_headers
  DotProduct.h
  Gradient.h
)

set(vectorcalculus_sources_device
  DotProduct.cxx
  Gradient.cxx
)

vtkm_library(
  NAME vtkm_filter_vectorcalculus
  HEADERS ${vectorcalculus_headers}
  DEVICE_SOURCES ${vectorcalculus_sources_device}
  USE_VTKM_JOB_POOL
)

set_property(TARGET
  vtkm_filter_vectorcalculus
  PROPERTY UNITY_BUILD_MODE GROUP
)

target_link_libraries(vtkm_filter_vectorcalculus PUBLIC vtkm_worklet vtkm_filter_common)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_vectorcalculus)

#add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
