mkdir subprojects
cd subprojects

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
# TODO Figure proper architecture path
GST_BIN_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/bin
GST_PKG_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib/pkgconfig
GST_C_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/include
GLIB_C_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/include/gio-unix-2.0
GLIB_C_PATH=$GLIB_C_PATH:$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib/glib-2.0/include
GLIB_C_PATH=$GLIB_C_PATH:$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/include/glib-2.0
CAIRO_C_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/include/cairo
WAYLAND_PKG_PATH=$SCRIPT_DIR/wayland/build/lib/x86_64-linux-gnu/pkgconfig/


#Build Gstreamer
git -C cerbero pull 2> /dev/null || git clone -b 1.21.2 https://gitlab.freedesktop.org/gstreamer/cerbero.git
cerbero/cerbero-uninstalled bootstrap
#TODO Build only required component for faster/lighter bootstrap
cerbero/cerbero-uninstalled package gstreamer-1.0

# #Build "dot"
# wget https://gitlab.com/api/v4/projects/4207231/packages/generic/graphviz-releases/7.0.1/graphviz-7.0.1.tar.gz
# tar -xf graphviz-7.0.1.tar.gz
# rm -rf graphviz-7.0.1.tar.gz
# cd graphviz-7.0.1
# rm -rf build
# mkdir build
# cd build
# PKG_CONFIG_PATH=$GST_PKG_PATH \
#     ../configure --prefix=$SCRIPT_DIR/graphviz-7.0.1/build/dist
# C_INCLUDE_PATH=$GLIB_C_PATH \
# PKG_CONFIG_PATH=$GST_PKG_PATH \
#     make -j12 # Not multithread friendly...
# make install
# cd .. 


# #Build Cairo w/ x11 backend (Gstreamer doesn't)
# # wget https://www.cairographics.org/releases/LATEST-cairo-1.16.0
# rm -rf cairo-1.16.0
# tar -xf LATEST-cairo-1.16.0
# # rm -rf LATEST-cairo-1.16.0
# cd cairo-1.16.0
# CFLAGS=-DCAIRO_HAS_XLIB_SURFACE=1 
# TODO no test or demo
# echo "path : C_INCLUDE_PATH=$GLIB_C_PATH"
# echo "pwd $(pwd)"
# # C_INCLUDE_PATH=$GLIB_C_PATH \
# PKG_CONFIG_PATH=$GST_PKG_PATH \
# ./configure --prefix=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64 --enable-xlib=yes --enable-xlib-xcb=yes
# make -j12
# make install

# Build Cairo with x11 backend
git -C cairo pull 2> /dev/null || git clone https://github.com/freedesktop/cairo.git
cd cairo
rm -rf build
PATH=$GST_BIN_PATH:$PATH \
PKG_CONFIG_PATH=$GST_PKG_PATH \
meson -Dtests=disabled -Dxlib-xcb=enabled \
    --prefix=$SCRIPT_DIR/cerbero/build/dist \
    --libdir=linux_x86_64/lib \
    --includedir=linux_x86_64/include \
    --bindir=linux_x86_64/bin \
    --libexecdir=linux_x86_64/libexec \
    --pkg-config-path=$GST_PKG_PATH \
    build
PATH=$GST_BIN_PATH:$PATH \
PKG_CONFIG_PATH=$GST_PKG_PATH \
C_INCLUDE_PATH=$GST_C_PATH:$GLIB_C_PATH:$CAIRO_C_PATH \
meson compile -C build
PATH=$GST_BIN_PATH:$PATH \
PKG_CONFIG_PATH=$GST_PKG_PATH \
C_INCLUDE_PATH=$GST_C_PATH:$GLIB_C_PATH:$CAIRO_C_PATH \
meson install -C build
cd ..


#Build wayland-scanner
git -C wayland pull 2> /dev/null || git clone -b 1.19.93 https://gitlab.freedesktop.org/wayland/wayland.git
cd wayland
rm -rf build
PKG_CONFIG_PATH=$GST_PKG_PATH \
meson -Dtests=false -Ddocumentation=false \
    --prefix=$SCRIPT_DIR/cerbero/build/dist/ \
    --libdir=linux_x86_64/lib \
    --includedir=linux_x86_64/include \
    --bindir=linux_x86_64/bin \
    --libexecdir=linux_x86_64/libexec \
    build
meson compile -C build
meson install -C build
cd ..

# #Build GTK3+
git -C gtk pull 2> /dev/null || git clone -b 3.24.34 https://gitlab.gnome.org/GNOME/gtk.git
cd gtk
rm -rf build
PATH=$GST_BIN_PATH:$PATH \
PKG_CONFIG_PATH=$GST_PKG_PATH \
meson -Dtests=false -Dintrospection=false -Ddemos=false -Dexamples=false \
    --prefix=$SCRIPT_DIR/cerbero/build/dist \
    --libdir=linux_x86_64/lib \
    --includedir=linux_x86_64/include \
    --bindir=linux_x86_64/bin \
    --libexecdir=linux_x86_64/libexec \
    --pkg-config-path=$GST_PKG_PATH \
    build
PATH=$GST_BIN_PATH:$PATH \
PKG_CONFIG_PATH=$GST_PKG_PATH \
C_INCLUDE_PATH=$GST_C_PATH:$GLIB_C_PATH:$CAIRO_C_PATH \
meson compile -C build
meson install -C build
cd ..

cd .. # Leave subprojects

echo " ###################################################################################"
echo " #"
echo " #  Example of launch an application built with these libraries"
echo " #"
echo " #     PKG_CONFIG_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib/pkgconfig \ "
echo " #     LD_LIBRARY_PATH=$SCRIPT_DIR/cerbero/build/dist/linux_x86_64/lib"
echo " #         dist/bin/onvifmgr "
echo " #"
echo " ###################################################################################"