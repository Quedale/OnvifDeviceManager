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

    ################################################################
    # 
    #    Build gudev-1.0 dependency
    #   sudo apt install libgudev-1.0-dev (tested 237)
    # 
    ################################################################
    PKG_UDEV=$SCRT_DIR/subprojects/systemd/build/dist/usr/lib/pkgconfig
    PKG_GUDEV=$SCRT_DIR/subprojects/libgudev/build/dist/lib/pkgconfig
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_GUDEV:$PKG_UDEV \
    pkg-config --exists --print-errors "gudev-1.0 >= 237"
    ret=$?
    if [ $ret != 0 ]; then 

        # TODO Check for python package
        # #libudev
        ##if run_command(python, '-c', 'import jinja2', check : false).returncode() != 0
        # git -C jinja pull 2> /dev/null || git clone -b 3.1.2 https://github.com/pallets/jinja.git
        # cd jinja
        # python3 setup.py install --user
        # cd ..

        PKG_LIBCAP=$SCRT_DIR/subprojects/libcap/dist/lib64/pkgconfig
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBCAP \
        pkg-config --exists --print-errors "libcap >= 2.53" # Or check for sys/capability.h
        ret=$?
        if [ $ret != 0 ]; then 
            git -C libcap pull 2> /dev/null || git clone -b libcap-2.53  git://git.kernel.org/pub/scm/linux/kernel/git/morgan/libcap.git
            cd libcap
            make -j$(nproc)
            make install DESTDIR=$(pwd)/dist
            cd ..
        else
            echo "libcap already found."
        fi


        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_UDEV \
        pkg-config --exists --print-errors "libudev >= 252" # Or check for sys/capability.h
        ret=$?
        if [ $ret != 0 ]; then 
            git -C systemd pull 2> /dev/null || git clone -b v252 https://github.com/systemd/systemd.git
            cd systemd
            rm -rf build
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBCAP \
            C_INCLUDE_PATH=$SCRT_DIR/subprojects/libcap/dist/usr/include \
            meson setup build \
            --default-library=static \
            -Dauto_features=disabled \
            -Dmode=developer \
            -Dlink-udev-shared=false \
            -Dlink-systemctl-shared=false \
            -Dlink-networkd-shared=false \
            -Dlink-timesyncd-shared=false \
            -Dlink-journalctl-shared=false \
            -Dlink-boot-shared=false \
            -Dfirst-boot-full-preset=false \
            -Dstatic-libsystemd=false \
            -Dstatic-libudev=true \
            -Dstandalone-binaries=false \
            -Dinitrd=false \
            -Dcompat-mutable-uid-boundaries=false \
            -Dnscd=false \
            -Ddebug-shell='' \
            -Ddebug-tty='' \
            -Dutmp=false \
            -Dhibernate=false \
            -Dldconfig=false \
            -Dresolve=false \
            -Defi=false \
            -Dtpm=false \
            -Denvironment-d=false \
            -Dbinfmt=false \
            -Drepart=false \
            -Dsysupdate=false \
            -Dcoredump=false \
            -Dpstore=false \
            -Doomd=false \
            -Dlogind=false \
            -Dhostnamed=false \
            -Dlocaled=false \
            -Dmachined=false \
            -Dportabled=false \
            -Dsysext=false \
            -Duserdb=false \
            -Dhomed=false \
            -Dnetworkd=false \
            -Dtimedated=false \
            -Dtimesyncd=false \
            -Dremote=false \
            -Dcreate-log-dirs=false \
            -Dnss-myhostname=false \
            -Dnss-mymachines=false \
            -Dnss-resolve=false \
            -Dnss-systemd=false \
            -Drandomseed=false \
            -Dbacklight=false \
            -Dvconsole=false \
            -Dquotacheck=false \
            -Dsysusers=false \
            -Dtmpfiles=false \
            -Dimportd=false \
            -Dhwdb=false \
            -Drfkill=false \
            -Dxdg-autostart=false \
            -Dman=false \
            -Dhtml=false \
            -Dtranslations=false \
            -Dinstall-sysconfdir=false \
            -Dseccomp=false \
            -Dselinux=false \
            -Dapparmor=false \
            -Dsmack=false \
            -Dpolkit=false \
            -Dima=false \
            -Dacl=false \
            -Daudit=false \
            -Dblkid=false \
            -Dfdisk=false \
            -Dkmod=false \
            -Dpam=false \
            -Dpwquality=false \
            -Dmicrohttpd=false \
            -Dlibcryptsetup=false \
            -Dlibcurl=false \
            -Didn=false \
            -Dlibidn2=false \
            -Dlibidn=false \
            -Dlibiptc=false \
            -Dqrencode=false \
            -Dgcrypt=false \
            -Dgnutls=false \
            -Dopenssl=false \
            -Dcryptolib=auto \
            -Dp11kit=false \
            -Dlibfido2=false \
            -Dtpm2=false \
            -Delfutils=false \
            -Dzlib=false \
            -Dbzip2=false \
            -Dxz=false \
            -Dlz4=false \
            -Dzstd=false \
            -Dxkbcommon=false \
            -Dpcre2=false \
            -Dglib=false \
            -Ddbus=false \
            -Dgnu-efi=false \
            -Dtests=false \
            -Durlify=false \
            -Danalyze=false \
            -Dbpf-framework=false \
            -Dkernel-install=false \
            --libdir=lib

            PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBCAP \
            C_INCLUDE_PATH=$SCRT_DIR/subprojects/libcap/dist/usr/include \
            LIBRARY_PATH=$SCRT_DIR/subprojects/libcap/dist/lib64 \
            meson compile -C build
            DESTDIR=$(pwd)/build/dist meson install -C build
            cd ..
        else
            echo "libudev already found."
        fi

        git -C libgudev pull 2> /dev/null || git clone -b 237 https://gitlab.gnome.org/GNOME/libgudev.git
        cd libgudev
        rm -rf build
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_UDEV \
        meson setup build \
            --default-library=static \
            -Dtests=disabled \
            -Dintrospection=disabled \
            -Dvapi=disabled \
            --prefix=$(pwd)/build/dist \
            --libdir=lib

        C_INCLUDE_PATH=$SCRT_DIR/subprojects/systemd/build/dist/usr/include \
        LIBRARY_PATH=$SCRT_DIR/subprojects/libcap/dist/lib64 \
        meson compile -C build
        meson install -C build
        cd ..

    else
        echo "gudev-1.0 already found."
    fi

    ################################################################
    # 
    #    Build v4l2-utils dependency
    #   sudo apt install libv4l2-dev (tested 1.16.3)
    # 
    ################################################################
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SCRT_DIR/subprojects/v4l-utils/dist/lib/pkgconfig \
    pkg-config --exists --print-errors "libv4l2 >= 1.16.3"
    ret=$?
    if [ $ret != 0 ]; then 
        git -C v4l-utils pull 2> /dev/null || git clone -b v4l-utils-1.22.1 https://github.com/gjasny/v4l-utils.git
        cd v4l-utils
        ./bootstrap.sh
        ./configure --prefix=$(pwd)/dist --enable-static --disable-shared --with-udevdir=$(pwd)/dist/udev
        make -j$(nproc)
        make install 
        cd ..
    else
        echo "libv4l2 already found."
    fi

    ################################################################
    # 
    #    Build alsa-lib dependency
    #   sudo apt install llibasound2-dev (tested 1.2.7.2)
    # 
    ################################################################
    PKG_ALSA=$SCRT_DIR/subprojects/alsa-lib/build/dist/lib/pkgconfig
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_ALSA \
    pkg-config --exists --print-errors "alsa >= 1.2.7.2"
    ret=$?
    if [ $ret != 0 ]; then  
        git -C alsa-lib pull 2> /dev/null || git clone -b v1.2.8 git://git.alsa-project.org/alsa-lib.git
        cd alsa-lib
        autoreconf -vif
        ./configure --prefix=$(pwd)/build/dist --enable-static=yes --enable-shared=yes
        make -j$(nproc)
        make install
        cd ..
    else
        echo "alsa-lib already found."
    fi

    ################################################################
    # 
    #    Build libpulse dependency
    #   sudo apt install libpulse-dev (tested 16.1)
    # 
    ################################################################
    PKG_PULSE=$SCRT_DIR/subprojects/pulseaudio/build/dist/lib/pkgconfig
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_PULSE \
    pkg-config --exists --print-errors "libpulse >= 16.1"
    ret=$?
    if [ $ret != 0 ]; then 
        PKG_LIBSNDFILE=$SCRT_DIR/subprojects/libsndfile/dist/lib/pkgconfig
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBSNDFILE \
        pkg-config --exists --print-errors "sndfile >= 1.2.0"
        ret=$?
        if [ $ret != 0 ]; then 
            git -C libsndfile pull 2> /dev/null || git clone -b 1.2.0 https://github.com/libsndfile/libsndfile.git
            cd libsndfile
            # autoreconf -vif
            cmake --target clean
            cmake -G "Unix Makefiles" \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_INSTALL_PREFIX="$(pwd)/dist" \
                -DBUILD_EXAMPLES=off \
                -DBUILD_TESTING=off \
                -DBUILD_SHARED_LIBS=on \
                -DINSTALL_PKGCONFIG_MODULE=on \
                -DINSTALL_MANPAGES=off 
            make -j$(nproc)
            make install
            cd ..
        else
            echo "sndfile already found."
        fi

        PKG_CHECK=$SCRT_DIR/subprojects/check/build/dist/lib/pkgconfig
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_CHECK \
        pkg-config --exists --print-errors "check >= 0.15.2"
        ret=$?
        if [ $ret != 0 ]; then 
            git -C check pull 2> /dev/null || git clone -b 0.15.2 https://github.com/libcheck/check.git
            cd check
            autoreconf -vif
            ./configure --prefix=$(pwd)/build/dist --enable-static=yes --enable-shared=no --enable-build-docs=no --enable-timeout-tests=no
            make -j$(nproc)
            make install
            cd ..
        else
            echo "check already found."
        fi

        git -C pulseaudio pull 2> /dev/null || git clone -b v16.1 https://gitlab.freedesktop.org/pulseaudio/pulseaudio.git
        cd pulseaudio
        rm -rf build
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBSNDFILE \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_CHECK \
        meson setup build \
            --default-library=static \
            -Ddaemon=false \
            -Ddoxygen=false \
            -Dman=false \
            -Dtests=false \
            -Ddatabase=simple \
            -Dbashcompletiondir=no \
            -Dzshcompletiondir=no \
            --prefix=$(pwd)/build/dist \
            --libdir=lib

        meson compile -C build
        meson install -C build
        cd ..
    else
        echo "libpulse already found."
    fi

    ################################################################
    # 
    #    Build libdrm dependency
    #   sudo apt install libdrm-dev 
    # 
    ################################################################
    PKG_DRM=$SCRT_DIR/subprojects/drm/build/dist/lib/pkgconfig
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_DRM \
    pkg-config --exists --print-errors "libdrm >= 2.4.114"
    ret=$?
    if [ $ret != 0 ]; then 
        git -C drm pull 2> /dev/null || git clone -b libdrm-2.4.114 https://gitlab.freedesktop.org/mesa/drm.git
        cd drm
        rm -rf build
        meson setup build \
            --default-library=static \
            -Dman-pages=disabled \
            -Dtests=false \
            -Dcairo-tests=disabled \
            --prefix=$(pwd)/build/dist \
            --libdir=lib

        meson compile -C build
        meson install -C build
        cd ..
        
    else
        echo "libdrm already found."
    fi

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
    MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:gdk-pixbuf=enabled"
    # MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:gtk3=enabled"
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
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_GUDEV:$PKG_ALSA:$PKG_PULSE:$PKG_UDEV \
    meson setup build \
    --buildtype=release \
    --strip \
    --default-library=static \
    --wrap-mode=forcefallback \
    -Dauto_features=disabled \
    $MESON_PARAMS \
    --prefix=$GST_DIR/build/dist \
    --libdir=lib
    LIBRARY_PATH=$LIBRARY_PATH:$SCRT_DIR/subprojects/systemd/build/dist/usr/lib \
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

        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_DRM \
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
        LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:$SCRT_DIR/subprojects/FFmpeg/dist/lib/pkgconfig \
        meson setup libav_build \
        --buildtype=release \
        --strip \
        --default-library=static \
        --wrap-mode=nofallback \
        -Dauto_features=disabled \
        $MESON_PARAMS \
        --prefix=$GST_DIR/libav_build/dist \
        --libdir=lib
        LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:$SCRT_DIR/subprojects/FFmpeg/dist/lib/pkgconfig \
        meson compile -C libav_build
        LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
        PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:$SCRT_DIR/subprojects/FFmpeg/dist/lib/pkgconfig \
        meson install -C libav_build

        #Remove shared lib to force static resolution to .a
        rm -rf $GST_DIR/libav_build/dist/lib/*.so
        rm -rf $GST_DIR/libav_build/dist/lib/gstreamer-1.0/*.so
    fi
    #Remove shared lib to force static resolution to .a
    rm -rf $GST_DIR/build/dist/lib/*.so
    rm -rf $GST_DIR/build/dist/lib/gstreamer-1.0/*.so
    cd ..
else
    rm -rf libav_build
fi

if [ $SKIP_GSOAP -eq 0 ]; then
    echo "-- Building gsoap libgsoap-dev --"
    #WS-Security depends on OpenSSL library 3.0 or 1.1
    wget https://sourceforge.net/projects/gsoap2/files/gsoap_2.8.123.zip/download
    unzip download
    rm download
    cd gsoap-2.8
    mkdir build
    ./configure --with-openssl=/usr/lib/ssl --prefix=$(pwd)/build
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