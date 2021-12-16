##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# TODO: migrate Lagragian*, StreamSurface

set(particle_advection_headers
  AdvectorBaseAlgorithm.h
  AdvectorBaseAlgorithm.hxx # FIXME: not used anywhere?
  AdvectorBaseThreadedAlgorithm.h
  BoundsMap.h
  DataSetIntegrator.h
  Messenger.h
  ParticleAdvectionAlgorithm.h
  ParticleMessenger.h
)

set(particle_advection_sources_device
  DataSetIntegrator.cxx
  Messenger.cxx
  ParticleAdvection.cxx
  ParticleMessenger.cxx
  Pathline.cxx
  PathParticle.cxx
  Streamline.cxx
  )

vtkm_library(
  NAME vtkm_filter_particle_advection
  HEADERS ${particle_advection_headers}
  DEVICE_SOURCES ${particle_advection_sources_device}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_particle_advection PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_particle_advection)

add_subdirectory(worklet)

if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
