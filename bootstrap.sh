SKIP_GSOAP=0
SKIP_DISCOVERY=0
SKIP_ONVIFLIB=0
WITH_GSTREAMER=0
i=1;
for arg in "$@" 
do
    if [ $arg == "--skip-gsoap" ]; then
        SKIP_GSOAP=1
    elif [ $arg == "--skip-discovery" ]; then
        SKIP_DISCOVERY=1
    elif [ $arg == "--skip-onviflib" ]; then
        SKIP_ONVIFLIB=1
    elif [ $arg == "--with-gstreamer" ]; then
        WITH_GSTREAMER=1
    fi
    i=$((i + 1));
done

sudo apt install automake autoconf gcc make pkg-config
sudo apt install libxml2-dev libgtk-3-dev
sudo apt install unzip
sudo apt install meson
sudo apt install libssl-dev
sudo apt install bison
sudo apt install flex

mkdir subprojects
cd subprojects
if [ $WITH_GSTREAMER -eq 1 ]; then
    git -C cerbero pull 2> /dev/null || git clone -b 1.21.2 https://gitlab.freedesktop.org/gstreamer/cerbero.git
    cerbero/cerbero-uninstalled bootstrap
    #TODO Build only required component for faster/lighter bootstrap
    cerbero/cerbero-uninstalled package gstreamer-1.0
    #./cerbero-uninstalled build gst-rtsp-server-1.0 gst-plugins-base-1.0
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
cd ..

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

echo "-- installing Gstreamer dependencies --"
sudo apt install libgstreamer1.0-dev #client
sudo apt install gstreamer1.0-libav #H264 decoder
sudo apt install gstreamer1.0-pulseaudio #pulsesink for client
sudo apt install libgstrtspserver-1.0-dev #server
sudo apt install gstreamer1.0-plugins-ugly #x264enc for server

aclocal
autoconf
automake --add-missing
autoreconf -i
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
