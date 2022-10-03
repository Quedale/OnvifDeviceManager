sudo apt install automake autoconf gcc make pkg-config
sudo apt install libxml2-dev libgtk-3-dev
#wget https://sourceforge.net/projects/gsoap2/files/gsoap_2.8.123.zip/download -P $(dirname "$0")/
echo "-- installing gsoap libgsoap-dev --"
sudo apt install gsoap libgsoap-dev

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


#TODO extract http://docs.oasis-open.org/wsn/b-2.xsd
#               http://docs.oasis-open.org/wsrf/bf-2.xsd
#               http://docs.oasis-open.org/wsn/t-1.xsd
rm -rf src/generated
# mkdir src/generated
# wsdl2h -c -o src/generated/device_service.h wsdl/device_service.wsdl
# soapcpp2 -CL -I/usr/share/gsoap/import src/device_service.h -dsrc/generated -x

# #TODO offline files
# mkdir src/generated
# wsdl2h -c -o src/generated/media_service.h wsdl/media.wsdl
# soapcpp2 -CL -I/usr/share/gsoap/import src/generated/media_service.h -dsrc/generated -x

mkdir src/generated
wsdl2h -c -o src/generated/common_service.h wsdl/*.wsdl
soapcpp2 -CL -I/usr/share/gsoap/import src/generated/common_service.h -dsrc/generated -x

# wget http://www.onvif.org/ver10/device/wsdl/devicemgmt.wsdl --d $(dirname "$0")/wsdl2/www.onvif.org.ver10.device.wsdl

# unzip gsoap_2.8.123.zip -d $(dirname "$0")/
# mkdir gsoap-build
# $(dirname "$0")/gsoap-2.8/configure --enable-samples --prefix=$(dirname "$0")/gsoap-build
# make
# make install