##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#vtkm_add_instantiations(ContourInstantiations FILTER Contour)

set(contour_headers
  ClipWithField.h
  Contour.h
#        ContourTreeUniformAugmented.h
#        ContourTreeUniformDistributed.h
#        ContourTreeUniform.h
  Slice.h
)

set(contour_header_template_sources
#        ContourTreeUniformAugmented.hxx
#        ContourTreeUniformDistributed.hxx
#        ContourTreeUniform.hxx
)

set(contour_sources_device
  ClipWithField.cxx
  Contour.cxx
#  ${ContourInstantiations}
  Slice.cxx
)

set_source_files_properties(Contour.cxx PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

vtkm_library(
  NAME vtkm_filter_contour
  TEMPLATE_SOURCES ${contour_header_template_sources}
  HEADERS ${contour_headers}
  DEVICE_SOURCES ${contour_sources_device}
  USE_VTKM_JOB_POOL
)

set_property(TARGET
  vtkm_filter_contour
  PROPERTY UNITY_BUILD_MODE GROUP
)

target_link_libraries(vtkm_filter_contour PUBLIC vtkm_filter_common)

if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_contour PUBLIC MPI::MPI_CXX)
endif ()

target_link_libraries(vtkm_filter PUBLIC INTERFACE
   vtkm_filter_contour
)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
