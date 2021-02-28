# About

**Why?** As a hiker, I want to plan, edit, and share tracks on interactive and immersive maps.

**What?** A desktop application with a 2D map for topographic analysis and a 3D map for terrain analysis.

**How?** The idea is to make a complete solution based on:

* The powerful route planning software: [QMapShack](https://github.com/Maproom/qmapshack/wiki),
* The immersive 3D map browser: [VTS frontend](https://github.com/melowntech/vts-browser-cpp),
* Probably the best maps with global coverage through the [Melown](https://www.melowntech.com/) and [ExploreWilder](https://explorewilder.com/) proxies.

# Work in Progress

This project is experimental.

# Build for GNU/Linux Desktop

Install packages that are required to build the application.

```
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

```
git clone -b 3D --recursive https://github.com/ExloreWilder/qmapshack
mkdir build_QMapShack
cd build_QMapShack
ccmake ../qmapshack
export CPLUS_INCLUDE_PATH=/usr/include/gdal:/usr/include/eigen3
export C_INCLUDE_PATH=/usr/include/gdal:/usr/include/eigen3
make --jobs=`nproc`
```

And run the application.

```
./bin/qmapshack
```

If you edit the program, re-compile:

```
export CPLUS_INCLUDE_PATH=/usr/include/gdal:/usr/include/eigen3
make --jobs=`nproc`
```
