include(${CMAKE_CURRENT_LIST_DIR}/lv_interop/CMakeLists.txt)

target_sources(g_industrial_cam PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/enumerate_devices.cpp
)