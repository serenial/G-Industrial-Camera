cmake_minimum_required(VERSION 3.27)

include(GenerateExportHeader)

# Find aravis from package config file
# Which requires the use of PkgConfig tool
find_package(PkgConfig)
pkg_check_modules(aravis REQUIRED IMPORTED_TARGET aravis-0.8)

# specify shared-library target
add_library(g_industrial_cam SHARED)

include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)

target_include_directories(g_industrial_cam
  PUBLIC
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/g_industrial_cam>"
  # add binary directory so auto-generated g_industrial_cam_export.h is found
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

# plaform specific definitions
if(WIN32)
  # windows.h definitions
  target_compile_definitions(g_industrial_cam PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX /wd4068)
elseif(UNIX)
  # link against static system libs
  target_link_options(g_industrial_cam PRIVATE
    "-static-libgcc" "-static-libstdc++"
  )

  # try and remove unused code sections
  target_compile_options(g_industrial_cam PRIVATE
    "-fdata-sections" "-ffunction-sections" "-Wl,--gc-sections"
  )

  # don't use "lib"-prefix on linux
  set_target_properties(g_industrial_cam PROPERTIES PREFIX "")
endif()

# specify target properties
set_property(TARGET g_industrial_cam PROPERTY CXX_STANDARD 20)

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
set_target_properties(g_industrial_cam PROPERTIES OUTPUT_NAME g_industrial_cam_${BITNESS}-${CMAKE_PROJECT_VERSION_MAJOR})
target_link_libraries(g_industrial_cam PRIVATE 
  PkgConfig::aravis
)

# Install Shared Library
INSTALL(TARGETS g_industrial_cam
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
)