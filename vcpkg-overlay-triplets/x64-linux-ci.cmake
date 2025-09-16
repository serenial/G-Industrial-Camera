# vcpkg\triplets\community\x64-linux-release.cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CRT_LINKAGE dynamic)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_BUILD_TYPE release)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# force static linking of libgcc and libstd
set(VCPKG_LINKER_FLAGS_RELEASE "-static-libgcc -static-libstdc++")
set(VCPKG_LINKER_FLAGS "-static-libgcc -static-libstdc++")
