# G Industrial Camera (🚧 Under Construction 🚧)

YOU get an industrial camera driver for LabVIEW and YOU get an industrial camera driver for LabVIEW.

## Licencing and Usage in Closed Source Applications

This repository contains code under different licences to respect the licences of upstream dependencies.

Any items within the `LGPL-2.1` directory are released under the LGPL-2.1 licence and are used to produce shared-binary `.dll` or `.so` file which can then linked to dynamically. 

The remaining content of this repository is released under 0-clause BSD licence.

### Usage in Closed Source Applications
The 0-clause BSD licenced LabVIEW code can be freely used in closed source applications. Ensure that you provide the end user with the appropriate licence notices and rights under the LGPL-2.1 licence for the `.dll` or `.so` components. 

## Supported Platforms
The LabVIEW code is written in LabVIEW 2020 SP1 and provides binaries for the following platforms:

| Platform | Supported |
|----------|-----------|
| Windows x86 | 🚧 |
| Windows x64 | 🚧 |
| Linux x64 | 🚧 |
| NI-LinuxRT x64 | 🚧❔ |
| NI-LinuxRT ARM | ❌ |
| MacOS (x86-64 or Apple Silicon) | ❌ |

## Developement Setup


### Required Tools
* CMake (27 or greater)
* Ninja build tools
* MSVC 2023 on Windows
* gcc 11 on Linux
* LabVIEW 2020 SP1


This respository uses git submodules to manage build tooling. When cloning ensure you use

```bash
git clone --recursive https://gitlab.com/serenial/g-industrial-camera.git
```

If you already have cloned the top-level repo then use
```bash
git submodule update --init --recursive
```

... from inside the cloned repository

## Building Shared Binaries

### Windows
Copy and modify the `x<Arch>-win-build.bat-example` files and update the extension to `.bat`.

Ensure you modify the `call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat` path to the location for your MSVC `vcvars` setups scripts on your system.

Alternatively if you are using `VSCode` you can use the integrated CMake extension to build but ensure you launch the `VSCode` instance from either the x86 or x64 developer command prompt to setup all the build tools.


### Linux-Desktop (Ubuntu)
```bash
sudo apt install autoconf libudev-dev
```
Ensure `vckpg/bootstrap-vcpkg.sh` has been called

Use the `x64-linux-desktop-build.sh` script to build the binaries.

Alternatively if you are using `VSCode` you can use the integrated CMake extension to build.

## Contributions
Open to contributions - please open an issue to discuss

## Usage

**ENSURE YOU HAVE INSTALED THE DRIVERS FOR THE DEVICE ON YOUR SYSTEM**

For example "Pylon" for Basler cameras.

### Windows libusb setup
Windows doesn't always play nice with libusb so look at the resources section to see guides on how to make the OS use a libusb compatible driver. #TODO - explain this properly

## TODO
- [ ] Enumerate Cameras
- [ ] Start Stream
- [ ] Stop Stream
- [ ] Capture Frame
- [ ] Documentation
- [ ] Distribution Packages

## Resources
* [Swapping USB3 Device Driver on Windows to use `libsub`](https://github.com/AravisProject/aravis/issues/431#issuecomment-1092243935) - note: Use Windows Device Manager to swap back to the original driver to see the Basler Device in Pylon again.