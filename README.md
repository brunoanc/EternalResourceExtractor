# EternalResourceExtractor
![Build Status](https://github.com/PowerBall253/EternalResourceExtractor/actions/workflows/build.yml/badge.svg)

Small C utility to extract files from a DOOM Eternal .resources file.

## Features
* Supports launching from command line with arguments, double clicking on it, and dragging and dropping.
* ~2-3x faster than QuickBMS on extracting files.
* Standalone, does not require any additional files.
* Light, only occupies 1 MB.
* Supports both Windows and Linux.

## Usage
The syntax for the CMD is:

```
EternalResourceExtractor.exe [path to .resources file] [out path]
```

You can also double click on it or drag and drop the .resources file to get started.

## Compiling
The project uses CMake to compile. It also needs MSVC or the MinGW toolchain on MSYS to compile on Windows.

First clone the repo by running:

```
git clone https://github.com/PowerBall253/EternalResourceExtractorCpp.git
```

Then, generate the makefile by running:
```
cd EternalResourceExtractorCpp
mkdir build
cd build
cmake .. # Append "-G 'MSYS Makefiles'" on MSYS
```

Finally, build with:
```
cmake --build . --config Release
```

The EternalResourceExtractor executable will be in the "build" folder in Linux/MinGW and in the "build/Release" folder in MSVC.

## Credits
* aluigi: For the QuickBMS resource extractor script for The New Colossus.
* One of infogram's friends: For editing the script to work with DOOM Eternal.
