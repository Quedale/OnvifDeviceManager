#Latest from ubuntu
FROM  ubuntu:latest

#installing package dependencies
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update
RUN apt install -y unzip wget git make bison flex libntlm0-dev libtool libgtk-3-dev g++ libssl-dev zlib1g-dev libasound2-dev libgudev-1.0-dev libpulse-dev cmake
RUN apt install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
RUN apt install -y alsa-base alsa-utils
RUN apt install -y pulseaudio

#pull source
RUN git clone https://github.com/Quedale/OnvifDeviceManager.git

#autogen source
RUN (cd OnvifDeviceManager && ./autogen.sh --prefix=$(pwd)/dist)

# RUN echo "MAKING"
RUN make -j$(nproc) -C OnvifDeviceManager

# Generating a universally unique ID for the container
RUN dbus-uuidgen > /etc/machine-id

#Disable GTK Accessiblity to hide Gtk warning
ENV NO_AT_BRIDGE=1

ENTRYPOINT /OnvifDeviceManager/onvifmgr
