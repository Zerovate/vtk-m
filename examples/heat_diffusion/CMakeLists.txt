##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION  3.13)

project(Diffusion  CXX)

find_package(VTKm  REQUIRED)

vtkm_find_gl(OPTIONAL GL GLUT GLEW)
include(CTest)
enable_testing()


add_executable(Diffusion heat_diffusion.cxx Diffusion_Filter.h hd_worklets.h parameters.h)
target_link_libraries(Diffusion PRIVATE vtkm_filter vtkm_io)
vtkm_add_target_information(Diffusion
        DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
        DEVICE_SOURCES heat_diffusion.cxx)

if(TARGET OpenGL::GL AND
        TARGET GLUT::GLUT AND
        TARGET GLEW::GLEW AND
        TARGET vtkm_rendering )
    target_link_libraries(Diffusion PRIVATE OpenGL::GL GLEW::GLEW GLUT::GLUT vtkm_rendering  )
    target_compile_definitions(Diffusion PRIVATE -DANIMATE  )
else()
    MESSAGE(STATUS "Either OpenGL::GL or GLUT::GLUT or GLEW::GLEW not found: animation disabled")
endif()


file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/Tests/test_*.cxx")
foreach(file ${files})
    string(REGEX REPLACE "(^.*/|\\.[^.]*$)" "" file_without_ext ${file})
    add_executable(${file_without_ext} ${file} Diffusion_Filter.h hd_worklets.h)
    target_link_libraries(${file_without_ext} PRIVATE vtkm_filter )
    vtkm_add_target_information(${file_without_ext}
            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
            DEVICE_SOURCES ${file})
    add_test(${file_without_ext}_SERIAL ${file_without_ext} -d Serial)
    add_test(${file_without_ext}_OMP ${file_without_ext} -d OpenMP)
    add_test(${file_without_ext}_TBB ${file_without_ext} -d TBB)
    add_test(${file_without_ext}_CUDA ${file_without_ext} -d Cuda)
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/${file_without_ext}_data ${CMAKE_CURRENT_BINARY_DIR}/${file_without_ext}_data  COPYONLY)
endforeach()

add_test(perf_Serial Diffusion -d Serial -p -s 20 -i 1)
add_test(perf_OpenMP Diffusion -d OpenMP -p -s 20 -i 1)
add_test(perf_TBB Diffusion -d TBB -p -s 20 -i 1)
add_test(perf_CUDA Diffusion -d CUDA -p -s 20 -i 1)
