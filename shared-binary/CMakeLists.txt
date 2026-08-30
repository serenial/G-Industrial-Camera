cmake_minimum_required(VERSION 3.27)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set the rpath to just be the same directory as the library when resolving non-system deps

set(CMAKE_BUILD_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

include(GenerateExportHeader)

# Find aravis from package config file
# Which requires the use of PkgConfig tool
find_package(PkgConfig)
pkg_check_modules(aravis REQUIRED IMPORTED_TARGET aravis-0.8)

find_package(boost_circular_buffer REQUIRED CONFIG)

# specify shared-library target
add_library(g_industrial_cam SHARED)

# add compile definitions
target_compile_definitions(g_industrial_cam PUBLIC G_INDUSTRIAL_CAM_BYTE_PACKING_${CMAKE_SIZEOF_VOID_P})

include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)

target_include_directories(g_industrial_cam
  PUBLIC
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/g_industrial_cam>"

  # add binary directory so auto-generated g_industrial_cam_export.h is found
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

# plaform specific definitions
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_compile_definitions(g_industrial_cam PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX /wd4068)

  if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    target_compile_definitions(g_industrial_cam PUBLIC G_INDUSTRIAL_CAM_BYTE_PACKING_WIN_x86)
  endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # try and remove unused code sections
  target_compile_options(g_industrial_cam PRIVATE
    "-fdata-sections" "-ffunction-sections" "-Wl,--gc-sections"
  )

  # don't use "lib"-prefix on linux
  set_target_properties(g_industrial_cam PROPERTIES PREFIX "")
else()
  message(FATAL_ERROR "Unsupported platform. Only Windows and Linux supported. Current CMAKE_SYSTEM_NAME is \"${CMAKE_SYSTEM_NAME}\"")
endif()

# specify target properties (C++17 maximum supported by ni-rt toolchains from 2018)
set_property(TARGET g_industrial_cam PROPERTY CXX_STANDARD 17)

set_target_properties(g_industrial_cam PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

# generate export header
generate_export_header(g_industrial_cam)

# Set the Library Filename to include the bitness
math(EXPR BITNESS "8*${CMAKE_SIZEOF_VOID_P}")
set_target_properties(g_industrial_cam PROPERTIES OUTPUT_NAME gindustrialcam_${BITNESS}-${CMAKE_PROJECT_VERSION_MAJOR})
target_link_libraries(g_industrial_cam PRIVATE
  PkgConfig::aravis
  Boost::circular_buffer
)

# Install Shared Library
INSTALL(TARGETS g_industrial_cam
  RUNTIME DESTINATION g_industrial_cam/bin
  LIBRARY DESTINATION g_industrial_cam/bin
)