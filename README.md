# Depths of Hoenn

An interactive underwater 3D scene built with C++ and OpenGL 4.0.  
Explore a bioluminescent coral reef, collect 5 Blue Orbs, and awaken the legendary Kyogre.

## Dependancies
* **CMake** 3.10+
* **GLFW**: 3.x window creation and input
* **GLAD**: OpenGL function loader
* **GLM**: Mathematics library
* **Assimp**: Asset Import Library (required to load .gltf and .obj models)
* **ZLIB**: Compression library (required by CMake setup)
* **STB Image**: Texture loading (header-only, included in project)

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

You are a diver exploring the depths of Hoenn and somewhere in the coral reef, 5 Blue Orbs lie hidden.
Ancient relics needed to awaken the Sea God Kyogre, who rests dormant behind you as a silent statue.

Follow Lumineon, the glowing guide fish as it swims toward the nearest uncollected orb. Once all 5 are found, Kyogre awakens.
