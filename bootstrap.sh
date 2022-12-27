#!/bin/bash
SKIP_GSOAP=0
SKIP_DISCOVERY=0
SKIP_ONVIFLIB=0
SKIP_GSTREAMER=0
ENABLE_LIBAV=0
i=1;

for arg in "$@" 
do
    shift
    if [ "$arg" == "--skip-gsoap" ]; then
        SKIP_GSOAP=1
    elif [ "$arg" == "--skip-discovery" ]; then
        SKIP_DISCOVERY=1
    elif [ "$arg" == "--skip-onviflib" ]; then
        SKIP_ONVIFLIB=1
    elif [ "$arg" == "--skip-gstreamer" ]; then
        SKIP_GSTREAMER=1
    elif [ "$arg" == "--enable-libav" ]; then
        ENABLE_LIBAV=1
        set -- "$@" "$arg"
    else
        set -- "$@" "$arg"
    fi
    i=$((i + 1));
done

#Save current working directory to run configure in
WORK_DIR=$(pwd)

#Get project root directory based on autogen.sh file location
SCRT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
cd $SCRT_DIR


mkdir -p $SCRT_DIR/subprojects
cd $SCRT_DIR/subprojects

if [ $SKIP_GSTREAMER -eq 0 ]; then
    git -C gstreamer pull 2> /dev/null || git clone -b 1.21.3 https://gitlab.freedesktop.org/gstreamer/gstreamer.git
    cd gstreamer

    GST_DIR=$(pwd)

    MESON_PARAMS=""
    #Force disable
    MESON_PARAMS="$MESON_PARAMS -Dglib:tests=false"
    MESON_PARAMS="$MESON_PARAMS -Dlibdrm:cairo-tests=false"
    MESON_PARAMS="$MESON_PARAMS -Dx264:cli=false"

    #Enabled features
    MESON_PARAMS="$MESON_PARAMS -Dbase=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgood=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dbad=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dugly=enabled" # To support x264
    MESON_PARAMS="$MESON_PARAMS -Dgpl=enabled" # To support openh264
    MESON_PARAMS="$MESON_PARAMS -Drtsp_server=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:app=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:typefind=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audiotestsrc=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:videotestsrc=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:playback=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:x11=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:xvideo=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:alsa=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:videoconvertscale=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:tcp=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:rawparse=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:pbtypes=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:overlaycomposition=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audioresample=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audioconvert=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:volume=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:v4l2=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtsp=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtp=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtpmanager=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:law=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:udp=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:autodetect=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:pulse=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:oss=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:oss4=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:interleave=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:audioparsers=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:level=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:openh264=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:nvcodec=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:v4l2codecs=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:fdkaac=enabled"
    # MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:tinyalsa=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:videoparsers=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:switchbin=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:onvif=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:mpegtsmux=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:mpegtsdemux=enabled"
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-ugly:x264=enabled"

    rm -rf build
    meson setup build \
    --buildtype=release \
    --strip \
    --default-library=static \
    --wrap-mode=forcefallback \
    -Dauto_features=disabled \
    $MESON_PARAMS \
    --prefix=$GST_DIR/build/dist \
    --libdir=lib
    meson compile -C build
    meson install -C build

    if [ $ENABLE_LIBAV -eq 1 ]; then
        cd ..
        #######################
        #
        # Custom FFmpeg build
        #   For some reason, Gstreamer's meson dep doesn't build any codecs
        #
        #######################
        git -C FFmpeg pull 2> /dev/null || git clone -b n5.1.2 https://github.com/FFmpeg/FFmpeg.git
        cd FFmpeg

        ./configure --prefix=$(pwd)/dist \
            --enable-libdrm \
            --disable-lzma \
            --disable-doc \
            --disable-shared \
            --enable-static \
            --enable-nonfree \
            --enable-version3 \
            --enable-gpl 
        make -j$(nproc)
        make install
        rm -rf dist/lib/*.so
        cd ..

        #######################
        #
        # Rebuild gstreamer with libav with nofallback
        #
        #######################
        #Rebuild with custom ffmpeg build
        cd gstreamer
        MESON_PARAMS="-Dlibav=enabled"
        rm -rf libav_build
        LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib/pkgconfig \
        meson setup libav_build \
        --buildtype=release \
        --strip \
        --default-library=static \
        --wrap-mode=nofallback \
        -Dauto_features=disabled \
        $MESON_PARAMS \
        --prefix=$GST_DIR/libav_build/dist \
        --libdir=lib
        LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib/pkgconfig \
        meson compile -C libav_build
        LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:/home/quedale/git/OnvifDeviceManager/subprojects/FFmpeg/dist/lib/pkgconfig \
        meson install -C libav_build

        #Remove shared lib to force static resolution to .a
        rm -rf $GST_DIR/libav_build/dist/lib/*.so
        rm -rf $GST_DIR/libav_build/dist/lib/gstreamer-1.0/*.so
    fi
    #Remove shared lib to force static resolution to .a
    rm -rf $GST_DIR/build/dist/lib/*.so
    rm -rf $GST_DIR/build/dist/lib/gstreamer-1.0/*.so
    cd ..
fi

if [ $SKIP_GSOAP -eq 0 ]; then
    echo "-- Building gsoap libgsoap-dev --"
    #WS-Security depends on OpenSSL library 3.0 or 1.1
    wget https://sourceforge.net/projects/gsoap2/files/gsoap_2.8.123.zip/download
    unzip download
    rm download
    cd gsoap-2.8
    mkdir build
    ./configure --with-openssl=/usr/lib/ssl --prefix=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/build
    LIBRARY_PATH="$(pkg-config --variable=libdir openssl):$LIBRARY_PATH" \
    LD_LIBRARY_PATH="$(pkg-config --variable=libdir openssl):$LD_LIBRARY_PATH" \
        make -j$(nproc)
    make install
    cd ..
fi

if [ $SKIP_DISCOVERY -eq 0 ]; then
    echo "-- Bootrap OnvifDiscoveryLib  --"
    git -C OnvifDiscoveryLib pull 2> /dev/null || git clone https://github.com/Quedale/OnvifDiscoveryLib.git
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/subprojects/gsoap-2.8 OnvifDiscoveryLib/bootstrap.sh --skip-gsoap
fi

if [ $SKIP_ONVIFLIB -eq 0 ]; then
    echo "-- Bootstrap OnvifSoapLib  --"
    git -C OnvifSoapLib pull 2> /dev/null || git clone https://github.com/Quedale/OnvifSoapLib.git
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/subprojects/gsoap-2.8 OnvifSoapLib/bootstrap.sh --skip-gsoap
fi
cd ..

aclocal
autoconf
automake --add-missing
autoreconf -i
# wget "https://git.savannah.gnu.org/gitweb/?p=autoconf-archive.git;a=blob_plain;f=m4/ax_subdirs_configure.m4" -O m4/ax_subdirs_configure.m4

#WSL Video (Assuming the host is setup)
#echo $'\n\nexport DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk \'{print $2}\'):0\n' >> ~/.bashrc
#echo "dbus_status=$(service dbus status)"  >> ~/.bashrc
#echo "if [[ $dbus_status = *\"is not running\"* ]]; then"  >> ~/.bashrc
#echo "  sudo service dbus --full-restart"  >> ~/.bashrc
#echo "fi"  >> ~/.bashrc
#source ~/.bashrc

#WSL Audio (Assuming the host is setup)
#sudo add-apt-repository ppa:therealkenc/wsl-pulseaudio
#sudo apt update
#echo $'\n'"export HOST_IP=\"\$(ip route |awk '/^default/{print \$3}')\""  >> ~/.bashrc
#echo "export PULSE_SERVER=\"tcp:\$HOST_IP\""  >> ~/.bashrc

cd $WORK_DIR
$SCRT_DIR/configure $@