set(VCPKG_TARGET_ARCHITECTURE arm) #(arm = arm32 for VCPKG)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_BUILD_TYPE release)

# Reference the main toolchain file
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../scripts/toolchains/nilrt-armv7a.cmake")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)