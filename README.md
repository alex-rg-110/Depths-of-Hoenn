# Depths of Hoenn

An interactive underwater 3D scene built with C++ and OpenGL 4.0.  
Explore a bioluminescent coral reef, collect 5 Blue Orbs, and awaken the legendary Kyogre.

## Dependencies

- **CMake** 3.10+
- **GLFW** 3.x — window creation and input
- **GLAD** — OpenGL function loader
- **GLM** — mathematics library
- **STB Image** — texture loading (header-only, included in project)

## Build Instructions

### macOS (Apple Silicon M1/M2)

```bash
brew install cmake glfw
git clone https://github.com/alex-rg-110/Depths-of-Hoenn.git
cd Depths-of-Hoenn
mkdir build && cd build
cmake ..
make
```

### Linux

```bash
sudo apt-get install cmake libglfw3-dev libglm-dev
git clone https://github.com/alex-rg-110/Depths-of-Hoenn.git
cd Depths-of-Hoenn
mkdir build && cd build
cmake ..
make
```

### Windows

Install dependencies via [vcpkg](https://github.com/microsoft/vcpkg):
```bash
vcpkg install glfw3 glad glm
git clone https://github.com/alex-rg-110/Depths-of-Hoenn.git
cd Depths-of-Hoenn
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

> Note: developed and tested on macOS Apple M1. Compatibility on Windows and Linux is not guaranteed but the code uses standard OpenGL 4.0.

## Run

From inside the `build` directory:
```bash
./src/main
```

## Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move |
| Mouse | Look around |
| R | Restart after winning |
| ESC | Quit |

## Objective

Collect all 5 Blue Orbs hidden in the coral reef.  
Follow Lumineon — it always swims toward the nearest orb.  
Once all orbs are collected, Kyogre awakens.
