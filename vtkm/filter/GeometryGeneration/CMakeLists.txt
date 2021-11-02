##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(geometrygeneration_headers
        Tetrahedralize.h)

set(geometrygeneration_sources_device
        Tetrahedralize.cxx)

vtkm_library(
        NAME vtkm_filter_geometrygeneration
        HEADERS ${geometrygeneration_headers}
        DEVICE_SOURCES ${geometrygeneration_sources_device}
        USE_VTKM_JOB_POOL
)

set_property(TARGET
        vtkm_filter_geometrygeneration
        PROPERTY UNITY_BUILD_MODE GROUP
        )

target_link_libraries(vtkm_filter_geometrygeneration PUBLIC vtkm_worklet vtkm_filter_common)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_geometrygeneration)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
    add_subdirectory(testing)
endif ()
