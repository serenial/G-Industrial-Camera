include(${CMAKE_CURRENT_LIST_DIR}/lv_interop/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/device-io/CMakeLists.txt)


target_sources(g_industrial_cam PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/enumerate-devices.cpp
    ${CMAKE_CURRENT_LIST_DIR}/camera-control.cpp
    ${CMAKE_CURRENT_LIST_DIR}/buffer-operations.cpp
)