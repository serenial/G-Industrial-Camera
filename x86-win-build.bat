set SCRIPT_DIR="%~dp0."
::REM echo off
set BUILD_TYPE=release
SET VCPKG_DEFAULT_TRIPLET=x86-windows-static

if "%1" == "" (
     echo *** Building release ***
     echo If a debug version is required supply the batch file with the argument "debug".
)
if "%1" == "debug" (
    set BUILD_TYPE=debug
    echo ** Building debug ***
)

cd %SCRIPT_DIR%

call .\vcpkg\bootstrap-vcpkg.bat

::REM vcpkg can create a cmd context with all the settings we need but we cannot interact with 
::REM it from a batch file so echo out all the variables and then set them in this cmd context

FOR /F "tokens=*" %%I in ('.\vcpkg\vcpkg env --triplet %VCPKG_DEFAULT_TRIPLET% set') DO @SET %%I

cmake --preset=windows-%BUILD_TYPE% && cmake --build --preset=windows-%BUILD_TYPE%-build --clean-first