cmake_minimum_required(VERSION 3.27)

# Find aravis
find_package(aravis CONFIG REQUIRED)

# specify shared-library target
add_library(g_ind_cam_aravis SHARED)

target_include_directories(g_ind_cam_aravis
  PUBLIC
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
  # add binary directory so auto-generated g_ind_cam_aravis_export.h is found
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

# plaform specific definitions
if(WIN32)
  # windows.h definitions
  target_compile_definitions(g_ind_cam_aravis PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX /wd4068)
elseif(UNIX)
  # link against static system libs
  target_link_options(g_ind_cam_aravis PRIVATE
    "-static-libgcc" "-static-libstdc++"
  )

  # try and remove unused code sections
  target_compile_options(g_ind_cam_aravis PRIVATE
    "-fdata-sections" "-ffunction-sections" "-Wl,--gc-sections"
  )

  # don't use "lib"-prefix on linux
  set_target_properties(g_ind_cam_aravis PROPERTIES PREFIX "")
endif()

# specify target properties
# winrt only supports C++17 std
set_property(TARGET g_ind_cam_aravis PROPERTY CXX_STANDARD 17)
set_target_properties(${target_name} PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

# generate export header
generate_export_header(g_ind_cam_aravis)

# Set the Library Filename to include the bitness
math(EXPR BITNESS "8*${CMAKE_SIZEOF_VOID_P}")
set_target_properties(g_ind_cam_aravis PROPERTIES OUTPUT_NAME bin.${BITNESS}_${CMAKE_PROJECT_VERSION_MAJOR})
target_link_libraries(g_ind_cam_aravis PRIVATE
  aravis::aravis
)