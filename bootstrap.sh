SKIP_GSOAP=0
SKIP_DISCOVERY=0
SKIP_ONVIFLIB=0
i=1;
for arg in "$@" 
do
    if [ $arg == "--skip-gsoap" ]; then
        SKIP_GSOAP=1
    elif [ $arg == "--skip-discovery" ]; then
        SKIP_DISCOVERY=1
    elif [ $arg == "--skip-onviflib" ]; then
        SKIP_ONVIFLIB=1
    fi
    i=$((i + 1));
done

sudo apt install automake autoconf gcc make pkg-config
sudo apt install libxml2-dev libgtk-3-dev
sudo apt-get install unzip

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
    echo "-- Building OnvifDiscoveryLib  --"
    git -C OnvifDiscoveryLib pull 2> /dev/null || git clone https://github.com/Quedale/OnvifDiscoveryLib.git
    #Out-of-tree build to keep src clean
    cd OnvifDiscoveryLib
    mkdir build
    cd build
    mkdir dist
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/../../gsoap-2.8 ./bootstrap.sh --skip-gsoap
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/../../gsoap-2.8 ../configure --prefix=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/dist
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/../../gsoap-2.8 make -j$(nproc)
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/../../gsoap-2.8 make install
    cd ..
    cd ..
fi

if [ $SKIP_ONVIFLIB -eq 0 ]; then
    echo "-- Building OnvifSoapLib  --"
    git -C OnvifSoapLib pull 2> /dev/null || git clone https://github.com/Quedale/OnvifSoapLib.git
    #Out-of-tree build to keep src clean
    cd OnvifSoapLib
    mkdir build
    cd build
    mkdir dist
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/../../ggsoap-2.8 ./bootstrap.sh --skip-gsoap
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/../../ggsoap-2.8 ../configure --prefix=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/dist
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/../../ggsoap-2.8 make -j$(nproc) onviflib
    GSOAP_SRC_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/../../gsoap-2.8 make install
    cd ..
    cd ..
fi

echo "-- installing Gstreamer dependencies --"
sudo apt install libgstreamer1.0-dev #client
sudo apt install gstreamer1.0-libav #H264 decoder
sudo apt install gstreamer1.0-pulseaudio #pulsesink for client
sudo apt install libgstrtspserver-1.0-dev #server
sudo apt install gstreamer1.0-plugins-ugly #x264enc for server

#For some reason, the backchannel server doesn't work out of the apt package.
#Setup from source
#git clone https://gitlab.freedesktop.org/gstreamer/cerbero.git
#cerbero/cerbero-uninstalled bootstrap
#cervero/cerbero-uninstalled package gstreamer-1.0
# PKG_CONFIG_PATH=/home/quedale/git/cerbero/build/dist/linux_x86_64/lib/pkgconfig
aclocal
autoconf
automake --add-missing
wget "https://git.savannah.gnu.org/gitweb/?p=autoconf-archive.git;a=blob_plain;f=m4/ax_subdirs_configure.m4" -O m4/ax_subdirs_configure.m4

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

# #TODO offline files
echo "Generating WSDL gsoap files..."
rm -rf src/generated
mkdir src/generated
gsoap-2.8/build/bin/wsdl2h -x -t wsdl/onvif-typemap.dat -o src/generated/common_service.h -c \
http://www.onvif.org/onvif/ver10/device/wsdl/devicemgmt.wsdl \
http://www.onvif.org/onvif/ver10/event/wsdl/event.wsdl \
http://www.onvif.org/onvif/ver10/display.wsdl \
http://www.onvif.org/onvif/ver10/deviceio.wsdl \
http://www.onvif.org/onvif/ver20/imaging/wsdl/imaging.wsdl \
http://www.onvif.org/onvif/ver10/media/wsdl/media.wsdl \
http://www.onvif.org/onvif/ver20/ptz/wsdl/ptz.wsdl \
http://www.onvif.org/onvif/ver10/receiver.wsdl \
http://www.onvif.org/onvif/ver10/recording.wsdl \
http://www.onvif.org/onvif/ver10/search.wsdl \
http://www.onvif.org/onvif/ver10/replay.wsdl \
http://www.onvif.org/onvif/ver20/analytics/wsdl/analytics.wsdl \
http://www.onvif.org/onvif/ver10/analyticsdevice.wsdl \
http://www.onvif.org/onvif/ver10/schema/onvif.xsd 
gsoap-2.8/build/bin/soapcpp2 -CL -x -Igsoap-2.8/gsoap/import:gsoap-2.8/gsoap src/generated/common_service.h -dsrc/generated
