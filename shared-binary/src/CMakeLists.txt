include(${CMAKE_CURRENT_LIST_DIR}/lv_interop/CMakeLists.txt)

target_sources(g_industrial_cam PUBLIC
  FILE_SET HEADERS
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
  FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/g_industrial_cam/aravis-error.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/g_industrial_cam/buffer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/g_industrial_cam/camera.hpp
)

target_sources(g_industrial_cam PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/aravis-error.cpp
    ${CMAKE_CURRENT_LIST_DIR}/buffer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/camera.cpp
    ${CMAKE_CURRENT_LIST_DIR}/enumerate_devices.cpp
    ${CMAKE_CURRENT_LIST_DIR}/snapshot.cpp
)