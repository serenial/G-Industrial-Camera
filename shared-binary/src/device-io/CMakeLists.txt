target_sources(g_industrial_cam PUBLIC
  FILE_SET HEADERS
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
  FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/g_industrial_cam/device-io/aravis-error.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/g_industrial_cam/device-io/buffer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/g_industrial_cam/device-io/camera.hpp
)

target_sources(g_industrial_cam PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/aravis-error.cpp
  ${CMAKE_CURRENT_LIST_DIR}/buffer.cpp
  ${CMAKE_CURRENT_LIST_DIR}/camera.cpp
)