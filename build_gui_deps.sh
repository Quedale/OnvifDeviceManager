#!/usr/bin/env bash

########################################
##
## This is a temporary implementation to handle GUI dependencies
## TODO : Possibly use cerbro
##
########################################

mkdir -p subprojects
cd subprojects

#Absolute path to current "subprojects" directory
SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)

#Variable used by utils.sh to build source trees
SOURCES=$SCRIPT_DIR
#Variable used by utils.sh to keep a clean cache copy
SRC_CACHE_DIR="$HOME/src_cache"
#Variable used by utils.sh to prompt after every build or skip it
CHECK=1
SKIP=1
#Include utils.sh functions
source $SCRIPT_DIR/../scripts/utils.sh

# TODO Figure proper architecture path
GST_BIN_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/bin
GST_TOOL_PATH=$SCRIPT_DIR/cerbero/build/build-tools/bin


#Using Cerbero built binaries first
PREFIX=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64
TOOLS_PREFIX=$SCRIPT_DIR/cerbero/build/build-tools

PATH=$GST_BIN_PATH:$GST_TOOL_PATH:$PATH
PYTHON_FOLDER=$(python3 -c 'import sys; version=sys.version_info[:3]; print("python{0}.{1}".format(*version))')
PYTHONPATH=$SCRIPT_DIR/cerbero/build/build-tools/lib/$PYTHON_FOLDER/site-packages
PYTHONUSERBASE=$PYTHONPATH
PKG_CONFIG_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib/pkgconfig
LD_LIBRARY_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib
LD_LIBRARY_PATH=$SCRIPT_DIR/cerbero/build/build-tools/lib:$LD_LIBRARY_PATH
LIBRARY_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib
LIBRARY_PATH=$SCRIPT_DIR/cerbero/build/build-tools/lib:$LIBRARY_PATH



#Build Gstreamer
git -C cerbero pull 2> /dev/null || git clone -b 1.21.2 https://gitlab.freedesktop.org/gstreamer/cerbero.git
cerbero/cerbero-uninstalled bootstrap
#TODO Build only required component for faster/lighter bootstrap
cerbero/cerbero-uninstalled package gstreamer-1.0


pullOrClone path="https://github.com/freedesktop/xorg-macros.git" tag="util-macros-1.19.1"
buildMake srcdir="xorg-macros" prefix="$TOOLS_PREFIX"


pullOrClone path="https://gitlab.freedesktop.org/xorg/proto/xcbproto.git" tag="xcb-proto-1.15.2"
buildMake srcdir="xcbproto" prefix="$PREFIX" datarootdir="$PREFIX/lib"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/pthread-stubs.git"
buildMake srcdir="pthread-stubs" prefix="$PREFIX"


pullOrClone path="https://gitlab.freedesktop.org/xorg/proto/xorgproto.git" tag="xorgproto-2022.2"
buildMake srcdir="xorgproto" prefix="$PREFIX" datarootdir="$PREFIX/lib"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libxau.git" tag="libXau-1.0.10"
buildMake srcdir="libxau" prefix="$PREFIX" autoreconf="true"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libxcb" tag="libxcb-1.15"
buildMake srcdir="libxcb" prefix="$PREFIX"

pullOrClone path="https://gitlab.freedesktop.org/wayland/wayland.git" tag="1.19.93" # 2.62.6 Cerbero recipe version
buildMeson srcdir="wayland" prefix="$PREFIX" mesonargs="-Dtests=false -Ddocumentation=false"


pullOrClone path="https://gitlab.freedesktop.org/wayland/wayland-protocols.git" tag="1.30" # 2.62.6 Cerbero recipe version
buildMeson srcdir="wayland-protocols" prefix="$PREFIX" mesonargs="-Dtests=false" datadir="lib"


pullOrClone path="https://github.com/xkbcommon/libxkbcommon.git" tag="xkbcommon-1.4.1" # 2.62.6 Cerbero recipe version
buildMeson srcdir="libxkbcommon" prefix="$PREFIX" mesonargs="-Denable-docs=false"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libxtrans.git" tag="xtrans-1.4.0" # 2.62.6 Cerbero recipe version
buildMake srcdir="libxtrans" prefix="$TOOLS_PREFIX"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libx11.git" tag="libX11-1.8.2" # 2.62.6 Cerbero recipe version
buildMake srcdir="libx11" prefix="$PREFIX"


pullOrClone path="https://github.com/freedesktop/libXext.git" tag="libXext-1.3.3" # 2.62.6 Cerbero recipe version
buildMake srcdir="libXext" prefix="$PREFIX"


pullOrClone path="https://github.com/freedesktop/cairo.git"
buildMeson srcdir="cairo" prefix="$PREFIX" mesonargs="-Dtests=disabled -Dxlib-xcb=enabled"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libxrender.git" tag="libXrender-0.9.11"
buildMake srcdir="libxrender" prefix="$PREFIX"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libxrandr.git" tag="libXrandr-1.5.3"
buildMake srcdir="libxrandr" prefix="$PREFIX"


pullOrClone path="https://gitlab.freedesktop.org/xorg/app/xrandr.git" tag="xrandr-1.5.1"
buildMake srcdir="xrandr" prefix="$PREFIX"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libxfixes.git" tag="libXfixes-6.0.0"
buildMake srcdir="libxfixes" prefix="$PREFIX"


pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libxi.git" tag="libXi-1.8"
buildMake srcdir="libxi" prefix="$PREFIX"


pullOrClone path="https://github.com/freedesktop/dbus.git" tag="dbus-1.15.2"
buildMeson srcdir="dbus" prefix="$PREFIX" mesonargs="-Dxml_docs=disabled -Dqt_help=disabled -Dmodular_tests=disabled -Dducktype_docs=disabled"


pullOrClone path="https://github.com/freedesktop/xorg-libXtst.git" tag="libXtst-1.2.4"
buildMake srcdir="xorg-libXtst" prefix="$PREFIX"


pullOrClone path="https://gitlab.gnome.org/GNOME/at-spi2-core.git" tag="AT_SPI2_CORE_2_42_1"
buildMeson srcdir="at-spi2-core" prefix="$PREFIX" mesonargs="-Ddocs=false"

pullOrClone path="https://gitlab.gnome.org/GNOME/atk.git" tag="2.38.0"
buildMeson srcdir="atk" prefix="$PREFIX"

pullOrClone path="https://gitlab.gnome.org/GNOME/at-spi2-atk.git" tag="AT_SPI2_ATK_2_38_0"
buildMeson srcdir="at-spi2-atk" prefix="$PREFIX" mesonargs="-Dtests=false"


# pullOrClone path="https://github.com/KhronosGroup/glslang.git" tag="11.12.0"
# mkdir -p "glslang/build"
# buildMake srcdir="glslang/build" prefix="$TOOLS_PREFIX" cmakedir=".."


# pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libpciaccess.git"
# buildMeson srcdir="libpciaccess" prefix="$PREFIX"

# pullOrClone path="https://gitlab.freedesktop.org/mesa/drm.git" tag="libdrm-2.4.114"
# buildMeson srcdir="drm" prefix="$PREFIX" mesonargs="-Dtests=false -Dintel=enabled"


# pullOrClone path="https://github.com/llvm/llvm-project.git" tag="llvmorg-15.0.5"
# mkdir -p "llvm-project/build"
# buildMake srcdir="llvm-project" prefix="$TOOLS_PREFIX" cmakedir=".." cmakeargs="-S llvm"

# pullOrClone path="https://gitlab.freedesktop.org/mesa/mesa.git" tag="mesa-22.2.4"
# buildMeson srcdir="mesa" prefix="$PREFIX"


pullOrClone path="https://github.com/sqlalchemy/mako.git" tag="rel_1_2_4"
cd mako
python3 setup.py install --prefix=$TOOLS_PREFIX 
cd ..

pullOrClone path="https://github.com/anholt/libepoxy.git" tag="1.5.10"
buildMeson srcdir="libepoxy" prefix="$PREFIX" mesonargs="-Dtests=false -Degl=yes -Dglx=yes"
#the "epoxy.pc" file is invalid if built without egl and glx. Removing invalid comma
sed -i 's/Requires.private: x11, /Requires.private: x11/' $SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib/pkgconfig/epoxy.pc

pullOrClone path="https://gitlab.gnome.org/GNOME/gtk.git" tag="3.24.34"
buildMeson srcdir="gtk" prefix="$PREFIX" mesonargs="-Dtests=false -Dintrospection=false -Ddemos=false -Dexamples=false -Dlibepoxy:tests=false"


cd .. # Leave subprojects

echo " ###################################################################################"
echo " #"
echo " #  Example of launch an application built with these libraries"
echo " #"
echo " #     PKG_CONFIG_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib/pkgconfig \\"
echo " #     LD_LIBRARY_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib \\"
echo " #         dist/bin/onvifmgr "
echo " #"
echo " ###################################################################################"