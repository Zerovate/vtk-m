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
  UnitTestStreamlineFilter.cxx
  )

set(libraries
  vtkm_filter_particle_advection
#  vtkm_filter_meshinfo # TODO: migrate GhostClassify
  vtkm_io)

if (VTKm_ENABLE_RENDERING)
  list(APPEND libraries vtkm_rendering)

  list(APPEND unit_tests
    RegressionTestStreamline.cxx
  )
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
)

if (VTKm_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestParticleMessengerMPI.cxx
    UnitTestStreamlineFilterMPI.cxx
    )
  vtkm_unit_tests(
    MPI SOURCES ${mpi_unit_tests}
    LIBRARIES ${libraries}
    USE_VTKM_JOB_POOL
  )
endif()
