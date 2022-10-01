sudo apt install automake autoconf gcc make pkg-config
sudo apt install libxml2-dev libgtk-3-dev
#wget https://sourceforge.net/projects/gsoap2/files/gsoap_2.8.123.zip/download -P $(dirname "$0")/
echo "-- installing gsoap libgsoap-dev --"
sudo apt install gsoap libgsoap-dev

echo "-- installing Gstreamer dependencies --"
sudo apt install libgstreamer1.0-dev #client
sudo apt install sudo apt-get install gstreamer1.0-libav #H264 decoder
sudo apt install gstreamer1.0-pulseaudio #pulsesink for client
sudo apt install libgstrtspserver-1.0-dev #server
sudo apt install gstreamer1.0-plugins-ugly #x264enc for server

aclocal
autoconf
automake --add-missing

#TODO extract http://docs.oasis-open.org/wsn/b-2.xsd
#               http://docs.oasis-open.org/wsrf/bf-2.xsd
#               http://docs.oasis-open.org/wsn/t-1.xsd
mkdir src/generated
wsdl2h -c -o src/generated/device_service.h wsdl/device_service.wsdl
soapcpp2 -CL -I/usr/share/gsoap/import src/generated/device_service.h -dsrc/generated


# wget http://www.onvif.org/ver10/device/wsdl/devicemgmt.wsdl --d $(dirname "$0")/wsdl2/www.onvif.org.ver10.device.wsdl

# unzip gsoap_2.8.123.zip -d $(dirname "$0")/
# mkdir gsoap-build
# $(dirname "$0")/gsoap-2.8/configure --enable-samples --prefix=$(dirname "$0")/gsoap-build
# make
# make install