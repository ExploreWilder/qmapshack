# About

**Why?** As a hiker, I want to plan, edit, and share tracks on interactive and immersive maps.

**What?** A desktop application with a 2D map for topographic analysis and a 3D map for terrain analysis.

**How?** The idea is to combine a powerful route planning software (QMapShack), a 3D map browser (VTS), and a tile server (ExploreWilder).

# Work in Progress

This project is experimental.

# Build for GNU/Linux Desktop

Install packages that are required to build the application.

```bash
sudo apt update
sudo apt install \
    git \
    g++ \
    cmake-curses-gui \
    nasm \
    libssl-dev \
    libboost-all-dev \
    libeigen3-dev \
    libproj-dev \
    libgeographic-dev \
    libjsoncpp-dev \
    libsdl2-dev \
    libfreetype6-dev \
    libharfbuzz-dev \
    libcurl4-openssl-dev \
    libjpeg-dev \
    libglfw3-dev \
    xorg-dev \
    qt5-default \
    qttools5-dev \
    qtwebengine5-dev \
    libgdal-dev \
    libroutino-dev \
    libquazip5-dev \
    libalglib-dev \
    doxygen
```

Clone the Git repository with all submodules and compile.

```bash
git clone -b 3D --recursive https://github.com/ExloreWilder/qmapshack
mkdir build_QMapShack
cd build_QMapShack
ccmake ../qmapshack
export CPLUS_INCLUDE_PATH=/usr/include/gdal
export C_INCLUDE_PATH=/usr/include/gdal
make --jobs=`nproc`
```

And run the application.

```bash
./bin/qmapshack
```
