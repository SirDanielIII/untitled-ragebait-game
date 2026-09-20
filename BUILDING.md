# 🚀 Building the Game

This project uses **CMake**, **MinGW/GCC**, and **raylib**.

> The game source lives in `game`.  
> VS Code and CLion use separate build folders under `build`.

---

## 📚 Table of Contents

- [1. Prerequisites](#1-prerequisites)
    - [C/C++ Compiler](#cc-compiler)
    - [CMake](#cmake)
- [2. Project Build Layout](#2-project-build-layout)
- [3. Visual Studio Code](#3-visual-studio-code)
    - [Install Extensions](#install-extensions)
    - [Open the Workspace](#open-the-workspace)
    - [Select CMake](#select-cmake)
    - [Select the Compiler](#select-the-compiler)
    - [Configure](#configure)
    - [Build](#build)
- [4. CLion](#4-clion)
    - [Open the Project](#open-the-project)
    - [Set the Toolchain](#set-the-toolchain)
    - [Set the Build Directory](#set-the-build-directory)
    - [Build and Run](#build-and-run)
- [5. Troubleshooting](#5-troubleshooting)
    - [`raylib.h` is not found](#raylibh-is-not-found)
    - [CMake cannot download raylib](#cmake-cannot-download-raylib)
    - [VS Code is using the wrong CMake](#vs-code-is-using-the-wrong-cmake)
    - [VS Code hides `.exe` files](#vs-code-hides-exe-files)
    - [Wrong build directory](#wrong-build-directory)
    - [PowerShell `where cmake` does nothing](#powershell-where-cmake-does-nothing)

---

# 1. Prerequisites

The examples in this guide use:

```text
C:\Tools
```

as the folder where development tools are stored.

You may use a different location, but update the paths in the examples accordingly.

## C/C++ Compiler

Install a Windows C/C++ compiler such as **MinGW-w64 / GCC**.

If you need a tutorial, you can watch this one: https://youtu.be/3OUu7t9nsFk.

The video uses **[WinLibs](https://winlibs.com/)** and extracts the MinGW folder into:

```text
C:\Tools\mingw64
```

After extracting it, add the compiler's `bin` folder to your Windows `PATH`:

```text
C:\Tools\mingw64\bin
```

Verify the installation:

```powershell
gcc --version
g++ --version
```

Both commands should print compiler version information.

---

## CMake

Download the "Windows x64 ZIP" version of CMake from their download page: https://cmake.org/download/

Extract the ZIP into your "Tools" directory.

For the examples in this guide, the extracted folder is **renamed to `cmake`**, giving:

```text
C:\Tools\cmake
```

The CMake executable should therefore be located at:

```text
C:\Tools\cmake\bin\cmake.exe
```

Add the CMake `bin` folder to your Windows `PATH`:

```text
C:\Tools\cmake\bin
```

Verify:

```powershell
cmake --version
```

You should see something similar to:

```text
cmake version 4.x.x
```

> [!IMPORTANT]
> WinLibs may include its own copy of CMake at:
>
> ```text
> C:\Tools\mingw64\bin\cmake.exe
> ```
>
> For this project, use the standalone Kitware CMake installation:
>
> ```text
> C:\Tools\cmake\bin\cmake.exe
> ```
>
> The compiler can still come from WinLibs.

With the example layout used throughout this guide:

```text
C:\Tools\
├── cmake\
│   └── bin\
│       └── cmake.exe
└── mingw64\
    └── bin\
        ├── gcc.exe
        └── g++.exe
```

A typical setup is therefore:

```text
CMake:
C:\Tools\cmake\bin\cmake.exe

C compiler:
C:\Tools\mingw64\bin\gcc.exe

C++ compiler:
C:\Tools\mingw64\bin\g++.exe
```

---

# 2. Project Build Layout

The CMake source directory is:

```text
game/
```

Each IDE gets its own build directory:

```text
build/
├── vscode/
└── clion/
```

This keeps CMake caches and generated files separate between IDEs.

The intended layout is:

```text
CISC_320_RagebaitGame/
├── assets/
├── build/
│   ├── vscode/
│   └── clion/
└── game/
    ├── CMakeLists.txt
    ├── src/
    ├── resources/
    └── projects/
```

> [!NOTE]
> raylib is downloaded automatically by CMake through `FetchContent` during the first successful configure.

---

# 3. Visual Studio Code

## Install Extensions

Install these Microsoft extensions:

- **C/C++**
- **CMake Tools**

---

## Open the Workspace

In VS Code, do:

```text
File → Open Workspace from File...
```

and open:

```text
game/projects/VSCode/main.code-workspace
```

The workspace should be configured to show the parent project directory so you can access `assets`, `build`, and `game` in one VS Code window.

---

## Select CMake

Open VS Code settings:

```text
Ctrl+Shift+P
→ Preferences: Open User Settings (JSON)
```

Make sure CMake Tools uses the standalone CMake installation and VSCode opens the correct directory by adding the following settings:

```json
{
  "cmake.cmakePath": "C:/Tools/cmake/bin/cmake.exe",
  "cmake.sourceDirectory": "${workspaceFolder}/game",
  "cmake.buildDirectory": "${workspaceFolder}/build/vscode"
}
```

> [!NOTE]
> `C:/Tools/...` is only the example location used in this guide. If you installed the tools somewhere else, use your own paths.

---

## Select the Compiler

Open the Command Palette:

```text
Ctrl+Shift+P
```

Run:

```text
CMake: Select a Kit
```

Choose the MinGW/GCC toolchain, for example:

```text
GCC x86_64-w64-mingw32
```

using:

```text
C:\Tools\mingw64\bin\gcc.exe
C:\Tools\mingw64\bin\g++.exe
```

---

## Configure

Run:

```text
Ctrl+Shift+P
→ CMake: Configure
```

VS Code should configure:

```text
game/ → build/vscode/
```

In the CMake output, confirm the command starts with:

```text
C:/Tools/cmake/bin/cmake.exe
```

and not:

```text
C:/Tools/mingw64/bin/cmake.exe
```

---

## Build

Use either:

- the **Build** button in the CMake Tools status bar (see bottom-left), or
- `Ctrl+Shift+P → CMake: Build`

The resulting files will be generated under:

```text
build/vscode/
```

---

# 4. CLion

## Open the Project

Open the parent project folder:

```text
CISC_320_RagebaitGame/
```

If CLion does not automatically detect the game target, locate:

```text
game/CMakeLists.txt
```

right-click the file, and load it as a CMake project.

---

## Set the Toolchain

This should be auto-detected, but in the case it isn't, go to:

```text
File
→ Settings
→ Build, Execution, Deployment
→ Toolchains
```

and select the MinGW/GCC compiler installed on your system.

Typical paths using the example setup in this guide:

```text
C:\Tools\mingw64\bin\gcc.exe
C:\Tools\mingw64\bin\g++.exe
```

---

## Set the Build Directory

Go to:

```text
File
→ Settings
→ Build, Execution, Deployment
→ CMake
```

And set the **Build directory** to:

```text
/CISC_320_RagebaitGame/build/clion
```

---

## Build and Run

Use CLion's normal:

- **Build**
- **Run**
- **Debug**

controls.

The generated build files will stay under:

```text
build/clion/
```

---

# 5. Troubleshooting

## `raylib.h` is not found

Before the first successful CMake configuration, the IDE may show `#include "raylib.h"` as missing.

This is expected because raylib is downloaded through CMake's `FetchContent`.

To fix it, run:

```text
CMake: Configure
```

After configuration, the IDE should receive the correct include paths.

---

## CMake cannot download raylib

If you see an SSL error such as:

```text
SSL Trust Anchors:
  no trust anchors configured

SSL certificate verification failed
```

the IDE is probably using the CMake executable bundled with WinLibs/MinGW.

Check available CMake executables:

```powershell
where.exe cmake
```

With the example installation used in this guide, prefer:

```text
C:\Tools\cmake\bin\cmake.exe
```

instead of:

```text
C:\Tools\mingw64\bin\cmake.exe
```

---

## VS Code is using the wrong CMake

Set:

```json
"cmake.cmakePath": "C:/Tools/cmake/bin/cmake.exe"
```

Then run:

```text
Ctrl+Shift+P
→ CMake: Delete Cache and Reconfigure
```

Check the CMake output and make sure the command starts with:

```text
C:/Tools/cmake/bin/cmake.exe
```

---

## VS Code hides `.exe` files

The raylib template may contain workspace settings that hide generated executables.

Check:

```json
"files.exclude"
```

inside the workspace or `.vscode/settings.json`.

Remove or disable any rule that hides:

```text
*.exe
```

---

## Wrong build directory

The expected layout is:

```text
build/
├── vscode/
└── clion/
```

VS Code should use:

```text
build/vscode/
```

CLion should use:

```text
build/clion/
```

Keeping them separate prevents one IDE from overwriting another IDE's CMake cache or generator files.

---

## PowerShell `where cmake` does nothing

In PowerShell, `where` is an alias for `Where-Object`.

Use:

```powershell
where.exe cmake
```

or:

```powershell
(Get-Command cmake).Source
```

instead.
