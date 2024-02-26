option(USE_MPI "Enable MPI-based distributed learning" OFF)
option(USE_OPENMP "Enable OpenMP" ON)
option(USE_GPU "Enable GPU-accelerated training" OFF)
option(USE_HDFS "Enable HDFS support (EXPERIMENTAL)" OFF)
option(USE_TIMETAG "Set to ON to output time costs" OFF)
option(USE_CUDA "Enable CUDA-accelerated training " OFF)
option(USE_DEBUG "Set to ON for Debug mode" OFF)
option(USE_SANITIZER "Use santizer flags" OFF)
set(
  ENABLED_SANITIZERS
  "address" "leak" "undefined"
  CACHE
  STRING
  "Semicolon separated list of sanitizer names, e.g., 'address;leak'. \
Supported sanitizers are address, leak, undefined and thread."
)
option(BUILD_CLI "Build the 'lightbgm' command-line interface in addition to lib_lightgbm" ON)
option(BUILD_STATIC_LIB "Build static library" OFF)
option(INSTALL_HEADERS "Install headers to CMAKE_INSTALL_PREFIX (e.g. '/usr/local/include')" OFF)
option(__BUILD_FOR_PYTHON "Set to ON if building lib_lightgbm for use with the Python package" OFF)
option(__BUILD_FOR_R "Set to ON if building lib_lightgbm for use with the R package" OFF)
option(__INTEGRATE_OPENCL "Set to ON if building LightGBM with the OpenCL ICD Loader and its dependencies included" OFF)

cmake_minimum_required(VERSION 3.18)

project(lgbm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

#-- Sanitizer
if(USE_SANITIZER)
  include(cmake/Sanitizer.cmake)
  enable_sanitizers("${ENABLED_SANITIZERS}")
endif()

if(__INTEGRATE_OPENCL)
  set(__INTEGRATE_OPENCL ON CACHE BOOL "" FORCE)
  set(USE_GPU OFF CACHE BOOL "" FORCE)
  message(STATUS "Building library with integrated OpenCL components")
endif()

if(__BUILD_FOR_PYTHON OR __BUILD_FOR_R)
    # the Python and R package don't require the CLI
    set(BUILD_CLI OFF)
    # installing the R and Python package shouldn't place LightGBM's headers
    # outside of where the package is installed
    set(INSTALL_HEADERS OFF)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.2")
    message(FATAL_ERROR "Insufficient gcc version")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.8")
    message(FATAL_ERROR "Insufficient Clang version")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.1.0")
    message(FATAL_ERROR "Insufficient AppleClang version")
  endif()
endif()

set(EIGEN_DIR "/opt/3rd/Eigen")
include_directories(SYSTEM ${EIGEN_DIR})
add_definitions(-DEIGEN_MPL2_ONLY)
add_definitions(-DEIGEN_DONT_PARALLELIZE)

if(__BUILD_FOR_R)
    find_package(LibR REQUIRED)
    message(STATUS "LIBR_EXECUTABLE: ${LIBR_EXECUTABLE}")
    message(STATUS "LIBR_INCLUDE_DIRS: ${LIBR_INCLUDE_DIRS}")
    message(STATUS "LIBR_CORE_LIBRARY: ${LIBR_CORE_LIBRARY}")
    include_directories(${LIBR_INCLUDE_DIRS})
    add_definitions(-DLGB_R_BUILD)
endif()

if(USE_TIMETAG)
    add_definitions(-DTIMETAG)
endif()

if(USE_DEBUG)
    add_definitions(-DDEBUG)
endif()

if(USE_MPI)
    find_package(MPI REQUIRED)
    add_definitions(-DUSE_MPI)
else()
    add_definitions(-DUSE_SOCKET)
endif()

if(USE_CUDA)
    set(CMAKE_CUDA_HOST_COMPILER "${CMAKE_CXX_COMPILER}")
    enable_language(CUDA)
    set(USE_OPENMP ON CACHE BOOL "CUDA requires OpenMP" FORCE)
endif()

if(USE_OPENMP)
    find_package(OpenMP REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

if(USE_GPU)
    set(BOOST_COMPUTE_HEADER_DIR ${PROJECT_SOURCE_DIR}/external_libs/compute/include)
    include_directories(${BOOST_COMPUTE_HEADER_DIR})
    find_package(OpenCL REQUIRED)
    include_directories(${OpenCL_INCLUDE_DIRS})
    message(STATUS "OpenCL include directory: " ${OpenCL_INCLUDE_DIRS})

    find_package(Boost 1.56.0 COMPONENTS filesystem system REQUIRED)

    include_directories(${Boost_INCLUDE_DIRS})
    add_definitions(-DUSE_GPU)
endif()

if(__INTEGRATE_OPENCL)
    include(cmake/IntegratedOpenCL.cmake)
    add_definitions(-DUSE_GPU)
endif()

if(USE_CUDA)
    find_package(CUDA 11.0 REQUIRED)
    include_directories(${CUDA_INCLUDE_DIRS})
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler=${OpenMP_CXX_FLAGS} -Xcompiler=-fPIC -Xcompiler=-Wall")

    # reference for mapping of CUDA toolkit component versions to supported architectures ("compute capabilities"):
    # https://en.wikipedia.org/wiki/CUDA#GPUs_supported
    set(CUDA_ARCHS "6.0" "6.1" "6.2" "7.0" "7.5")
    if(CUDA_VERSION VERSION_GREATER_EQUAL "11.0")
        list(APPEND CUDA_ARCHS "8.0")
    endif()
    if(CUDA_VERSION VERSION_GREATER_EQUAL "11.1")
        list(APPEND CUDA_ARCHS "8.6")
    endif()
    if(CUDA_VERSION VERSION_GREATER_EQUAL "11.5")
        list(APPEND CUDA_ARCHS "8.7")
    endif()
    if(CUDA_VERSION VERSION_GREATER_EQUAL "11.8")
        list(APPEND CUDA_ARCHS "8.9")
        list(APPEND CUDA_ARCHS "9.0")
    endif()
    list(POP_BACK CUDA_ARCHS CUDA_LAST_SUPPORTED_ARCH)
    list(APPEND CUDA_ARCHS "${CUDA_LAST_SUPPORTED_ARCH}+PTX")
    cuda_select_nvcc_arch_flags(CUDA_ARCH_FLAGS ${CUDA_ARCHS})
    string(REPLACE ";" " " CUDA_ARCH_FLAGS "${CUDA_ARCH_FLAGS}")

    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${CUDA_ARCH_FLAGS}")
    if(USE_DEBUG)
      set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -g")
    else()
      set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3 -lineinfo")
    endif()
    message(STATUS "CMAKE_CUDA_FLAGS: ${CMAKE_CUDA_FLAGS}")

    add_definitions(-DUSE_CUDA)

    if(NOT DEFINED CMAKE_CUDA_STANDARD)
      set(CMAKE_CUDA_STANDARD 11)
      set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    endif()

    set(
      BASE_DEFINES
      -DPOWER_FEATURE_WORKGROUPS=12
      -DUSE_CONSTANT_BUF=0
    )
    set(
      ALLFEATS_DEFINES
      ${BASE_DEFINES}
      -DENABLE_ALL_FEATURES
    )
    set(
      FULLDATA_DEFINES
      ${ALLFEATS_DEFINES}
      -DIGNORE_INDICES
    )

    message(STATUS "ALLFEATS_DEFINES: ${ALLFEATS_DEFINES}")
    message(STATUS "FULLDATA_DEFINES: ${FULLDATA_DEFINES}")

    function(add_histogram hsize hname hadd hconst hdir)
      add_library(histo${hsize}${hname} OBJECT src/treelearner/kernels/histogram${hsize}.cu)
      set_target_properties(histo${hsize}${hname} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
      set_target_properties(histo${hsize}${hname} PROPERTIES CUDA_ARCHITECTURES OFF)
      if(hadd)
        list(APPEND histograms histo${hsize}${hname})
        set(histograms ${histograms} PARENT_SCOPE)
      endif()
      target_compile_definitions(
        histo${hsize}${hname}
        PRIVATE
        -DCONST_HESSIAN=${hconst}
        ${hdir}
      )
    endfunction()

    foreach(hsize _16_64_256)
      add_histogram("${hsize}" "_sp_const" "True" "1" "${BASE_DEFINES}")
      add_histogram("${hsize}" "_sp" "True" "0" "${BASE_DEFINES}")
      add_histogram("${hsize}" "-allfeats_sp_const" "False" "1" "${ALLFEATS_DEFINES}")
      add_histogram("${hsize}" "-allfeats_sp" "False" "0" "${ALLFEATS_DEFINES}")
      add_histogram("${hsize}" "-fulldata_sp_const" "True" "1" "${FULLDATA_DEFINES}")
      add_histogram("${hsize}" "-fulldata_sp" "True" "0" "${FULLDATA_DEFINES}")
    endforeach()
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <xmmintrin.h>
int main() {
  int a = 0;
  _mm_prefetch(&a, _MM_HINT_NTA);
  return 0;
}
" MM_PREFETCH)

if(${MM_PREFETCH})
  message(STATUS "Using _mm_prefetch")
  add_definitions(-DMM_PREFETCH)
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <mm_malloc.h>
int main() {
  char *a = (char*)_mm_malloc(8, 16);
  _mm_free(a);
  return 0;
}
" MM_MALLOC)

if(${MM_MALLOC})
  message(STATUS "Using _mm_malloc")
  add_definitions(-DMM_MALLOC)
endif()

if(UNIX OR MINGW OR CYGWIN)
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -pthread -Wextra -Wall -Wno-ignored-attributes -Wno-unknown-pragmas -Wno-return-type"
  )
  if(USE_DEBUG)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
  else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  endif()
  if(NOT USE_OPENMP)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas -Wno-unused-private-field")
  endif()
  if(__BUILD_FOR_R AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-cast-function-type")
  endif()
endif()

if(NOT BUILD_STATIC_LIB)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()
if(NOT USE_DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops")
endif()

set(LightGBM_HEADER_DIR ${PROJECT_SOURCE_DIR}/include)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

include_directories(${LightGBM_HEADER_DIR})

if(USE_MPI)
  include_directories(${MPI_CXX_INCLUDE_PATH})
endif()

file(
    GLOB
    SOURCES
      src/boosting/*.cpp
      src/io/*.cpp
      src/metric/*.cpp
      src/objective/*.cpp
      src/network/*.cpp
      src/treelearner/*.cpp
      src/utils/*.cpp
)
file(
    GLOB
    LGBM_CUDA_SOURCES
      src/treelearner/*.cu
      src/boosting/cuda/*.cpp
      src/boosting/cuda/*.cu
      src/metric/cuda/*.cpp
      src/metric/cuda/*.cu
      src/objective/cuda/*.cpp
      src/objective/cuda/*.cu
      src/treelearner/cuda/*.cpp
      src/treelearner/cuda/*.cu
      src/io/cuda/*.cu
      src/io/cuda/*.cpp
      src/cuda/*.cpp
      src/cuda/*.cu
)

if(USE_CUDA)
  list(APPEND SOURCES ${LGBM_CUDA_SOURCES})
endif()

add_library(lightgbm_objs OBJECT ${SOURCES})

if(BUILD_CLI)
    add_executable(lightgbm src/main.cpp src/application/application.cpp)
    target_link_libraries(lightgbm PRIVATE lightgbm_objs)
endif()

set(API_SOURCES "src/c_api.cpp")
# Only build the R part of the library if building for
# use with the R package
if(__BUILD_FOR_R)
  list(APPEND API_SOURCES "src/lightgbm_R.cpp")
endif()

add_library(lightgbm_capi_objs OBJECT ${API_SOURCES})

if(BUILD_STATIC_LIB)
  add_library(_lightgbm STATIC)
else()
  add_library(_lightgbm SHARED)
endif()
# LightGBM headers include openmp, cuda, R etc. headers,
# thus PUBLIC is required for building _lightgbm_swig target.
target_link_libraries(_lightgbm PUBLIC lightgbm_capi_objs lightgbm_objs)

if(USE_MPI)
  target_link_libraries(lightgbm_objs PUBLIC ${MPI_CXX_LIBRARIES})
endif()

if(USE_OPENMP)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_link_libraries(lightgbm_objs PUBLIC OpenMP::OpenMP_CXX)
    # c_api headers also includes OpenMP headers, thus compiling
    # lightgbm_capi_objs needs include directory for OpenMP.
    # Specifying OpenMP in target_link_libraries will get include directory
    # requirements for compilation.
    # This uses CMake's Transitive Usage Requirements. Refer to CMake doc:
    # https://cmake.org/cmake/help/v3.16/manual/cmake-buildsystem.7.html#transitive-usage-requirements
    target_link_libraries(lightgbm_capi_objs PUBLIC OpenMP::OpenMP_CXX)
  endif()
endif()

if(USE_GPU)
  target_link_libraries(lightgbm_objs PUBLIC ${OpenCL_LIBRARY} ${Boost_LIBRARIES})
endif()

if(__INTEGRATE_OPENCL)
  # targets OpenCL and Boost are added in IntegratedOpenCL.cmake
  add_dependencies(lightgbm_objs OpenCL Boost)
  # variables INTEGRATED_OPENCL_* are set in IntegratedOpenCL.cmake
  target_include_directories(lightgbm_objs PRIVATE ${INTEGRATED_OPENCL_INCLUDES})
  target_compile_definitions(lightgbm_objs PRIVATE ${INTEGRATED_OPENCL_DEFINITIONS})
  target_link_libraries(lightgbm_objs PUBLIC ${INTEGRATED_OPENCL_LIBRARIES} ${CMAKE_DL_LIBS})
endif()

if(USE_CUDA)
  # Disable cmake warning about policy CMP0104. Refer to issue #3754 and PR #4268.
  # Custom target properties does not propagate, thus we need to specify for
  # each target that contains or depends on cuda source.
  set_target_properties(lightgbm_objs PROPERTIES CUDA_ARCHITECTURES OFF)
  set_target_properties(_lightgbm PROPERTIES CUDA_ARCHITECTURES OFF)
  if(BUILD_CLI)
    set_target_properties(lightgbm PROPERTIES CUDA_ARCHITECTURES OFF)
  endif()

  set_target_properties(lightgbm_objs PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

  # Device linking is not supported for object libraries.
  # Thus we have to specify them on final targets.
  if(BUILD_CLI)
    set_target_properties(lightgbm PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS ON)
  endif()
  set_target_properties(_lightgbm PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS ON)

  # histograms are list of object libraries. Linking object library to other
  # object libraries only gets usage requirements, the linked objects won't be
  # used. Thus we have to call target_link_libraries on final targets here.
  if(BUILD_CLI)
    target_link_libraries(lightgbm PRIVATE ${histograms})
  endif()
  target_link_libraries(_lightgbm PRIVATE ${histograms})
endif()


if(__BUILD_FOR_R)
  # utils/log.h and capi uses R headers, thus both object libraries need to link
  # with R lib.

  set(R_LIB ${LIBR_CORE_LIBRARY})
  target_link_libraries(lightgbm_objs PUBLIC ${R_LIB})
  target_link_libraries(lightgbm_capi_objs PUBLIC ${R_LIB})
endif()

if(BUILD_CLI)
    install(
      TARGETS lightgbm
      RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
endif()

if(__BUILD_FOR_PYTHON)
    set(CMAKE_INSTALL_PREFIX "lightgbm")
endif()

install(
  TARGETS _lightgbm
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

if(INSTALL_HEADERS)
    install(DIRECTORY ${LightGBM_HEADER_DIR}/LightGBM DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()
