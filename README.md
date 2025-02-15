# G Industrial Camera

YOU get an industrial camera driver for LabVIEW and YOU get an industrial camera driver for LabVIEW.

## Licencing

This repository contains code licenced under two different methods to respect the licences of upstream dependencies.

Any items within the `LGPL-2.0` directory are released under the LGPL-2.0 licence.

The remaining content of this repository is released under 0-clause BSD licence.

## Supported Platforms
The LabVIEW code is written in LabVIEW 2020 SP1 and provides binaries for the following platforms:

| Platform | Supported |
|----------|-----------|
| Windows x86 | ❌ |
| Windows x64 | ❌ |
| Linux x64 | ❌ |
| NI-LinuxRT x64 | ❌ |
| NI-LinuxRT ARM | ❌ |
| MacOS (x86-64 or Apple Silicon) | ❌ |

## Developement Setup

### Required Tools
CMake (27 or greater)
MSVC 2023 on Windows
gcc 11 on Linux
LabVIEW 2020


This respository uses git submodules to manage build tooling. When cloning ensure you use

```bash
git clone --recursive https://gitlab.com/serenial/g-industrial-camera.git
```

If you already have cloned the top-level repo then use
```bash
git submodule update --init --recursive
```

... from inside the cloned repository


