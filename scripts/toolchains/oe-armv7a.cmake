# based on vcpkg/scripts/toolchains/linux.cmake
# values from usr/local/oecore-x86_64/environment-setup-armv7a-vfp-oe-linux-gnueabi

if(NOT _OE_ARM3V7_A_TOOLCHAIN)
    set(_OE_ARM3V7_A_TOOLCHAIN TRUE)

    set(CMAKE_SYSTEM_NAME Linux CACHE STRING "")
    set(CMAKE_SYSTEM_PROCESSOR armv7a-vfp)

    set(CMAKE_CROSSCOMPILING ON)

    # Compile tools location
    set(OE_COMPILE_TOOLS_DIR "/usr/local/oecore-x86_64")

    # Sysroot configuration
    set(SDKTARGETSYSROOT "${OE_COMPILE_TOOLS_DIR}/sysroots/armv7a-vfp-oe-linux-gnueabi")
    set(OECORE_NATIVE_SYSROOT "${OE_COMPILE_TOOLS_DIR}/sysroots/x86_64-oesdk-linux")
    set(CMAKE_SYSROOT ${SDKTARGETSYSROOT})
    set(CMAKE_FIND_ROOT_PATH ${SDKTARGETSYSROOT})

    # Cross-compiler paths
    set(CROSS_COMPILE_PREFIX "arm-oe-linux-gnueabi-")
    set(TOOLCHAIN_BIN_DIR "${OECORE_NATIVE_SYSROOT}/usr/bin/arm-oe-linux-gnueabi")

    set(TOOLCHAIN_EXECUTABLE_PREFIX "${TOOLCHAIN_BIN_DIR}/${CROSS_COMPILE_PREFIX}")


    set(CMAKE_C_COMPILER "${TOOLCHAIN_EXECUTABLE_PREFIX}gcc")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_EXECUTABLE_PREFIX}g++")
    # Don't specify the ASM compiler as it won't know what to do with the --SYSROOTS arg
    # and CMAKE will use the C_COMPILER
    # set(CMAKE_ASM_COMPILER "${TOOLCHAIN_EXECUTABLE_PREFIX}as")

    # Binutils
    set(CMAKE_AR "${TOOLCHAIN_EXECUTABLE_PREFIX}ar" CACHE FILEPATH "Archiver")
    set(CMAKE_RANLIB "${TOOLCHAIN_EXECUTABLE_PREFIX}ranlib" CACHE FILEPATH "Ranlib")
    set(CMAKE_STRIP "${TOOLCHAIN_EXECUTABLE_PREFIX}strip" CACHE FILEPATH "Strip")
    set(CMAKE_NM "${TOOLCHAIN_EXECUTABLE_PREFIX}nm" CACHE FILEPATH "NM")
    set(CMAKE_OBJCOPY "${TOOLCHAIN_EXECUTABLE_PREFIX}objcopy" CACHE FILEPATH "Objcopy")
    set(CMAKE_OBJDUMP "${TOOLCHAIN_EXECUTABLE_PREFIX}objdump" CACHE FILEPATH "Objdump")

    # Compiler and Linker Flags
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_C_FLAGS_INIT "-march=armv7-a -mfpu=vfp -mfloat-abi=softfp -O3 -fPIC")
    set(CMAKE_CXX_FLAGS_INIT "-march=armv7-a -mfpu=vfp -mfloat-abi=softfp -O3 -fPIC")

    set(CMAKE_MODULE_LINKER_FLAGS_INIT "-Wl,-O1")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-Wl,-O1")
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-O1")

endif()