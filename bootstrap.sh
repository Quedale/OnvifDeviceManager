aclocal
autoconf
automake --add-missing
wget https://sourceforge.net/projects/gsoap2/files/gsoap_2.8.123.zip/download -P $(dirname "$0")/

echo "-- installing gsoap gsoap-dev --"
sudo apt install gsoap gsoap-dev

# wget http://www.onvif.org/ver10/device/wsdl/devicemgmt.wsdl --d $(dirname "$0")/wsdl2/www.onvif.org.ver10.device.wsdl

# unzip gsoap_2.8.123.zip -d $(dirname "$0")/
# mkdir gsoap-build
# $(dirname "$0")/gsoap-2.8/configure --enable-samples --prefix=$(dirname "$0")/gsoap-build
# make
# make install