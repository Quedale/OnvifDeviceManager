# OnvifDeviceManager
Onvif Device Manager for Linux

![Application Capture](images/AppCapture.png?raw=true "OnvifDeviceMgr Linux")

# Description
The goal of this project is to implement a Onvif Device Manager similar to the windows client, compatible for linux. I'm also working on adding some Profile T capabilities, such as bidirectional audio.

# Working
- Onvif WS-Discovery (gsoap)
- Soap Client for Onvif Device and Media service
- Vew RTSP Stream with backchannel (Push-to-talk)
- Prototype Soundlevel indicator
- WS-Security
- Support system and static libraries. (static recommended)
- Support Multiple ONVIF Profiles

# TODO
- EventQueue : Interupt pending events when needed
- Support MJPEG stream
- Credential Storage 
- Edit Onvif device identification
- Display Onvif network information
- Display Media information
- Better Look&Feel
- Testing with a variety of camera
- Record video
- JPEG Snapshot
- Check for better backchannel audio support. (PCMU@8000 might be Onvif's spec limit)
- And a lot more...

# Use-case
1. Doorbell-like security camera terminal
2. Babymonitor terminal (I hope to add common baby monitor feature like playing music)
3. Cross-room communication terminal

 
 
# How to build
### Clone repository
```
git clone https://github.com/Quedale/OnvifDeviceManager.git
cd OnvifDeviceManager
```
### Autogen, Configure, Download and build dependencies
autogen.sh will attempt download and build missing dependencies.   
**[Mandatory]** The following package dependencies are mandatory and are not yet automatically built:
```
sudo apt install make
sudo apt install bison 
sudo apt install flex 
sudo apt install libtool 
sudo apt install pkg-config
sudo apt install libgtk-3-dev
sudo apt install g++
```
**[Optional]** The following package are optional, but will reduce the runtime of autogen.sh if installed.
```
sudo apt install python3-pip
python3 -m pip install meson
python3 -m pip install ninja
sudo apt install libssl-dev
sudo apt install zlib1g-dev
sudo apt install libasound2-dev
sudo apt install libgudev-1.0-dev
sudo apt install gettext
sudo apt install libpulse-dev
sudo apt install nasm
```
If your system already has gstreamer pre-installed, I strongly recommend using `--enable-latest` to download the latest gstreamer release supported.   
Note that autogen will automatically call "./configure".
```
./autogen.sh --prefix=$(pwd)/dist --enable-latest
```

### Compile and install GUI App
```
make
make install
```
# Note
I have very little spare time to work on any personal project, so this might take a while.
This is my very first C project, so I'm learning as I go. 
 
# Known issues
1. GTKComboBox intercept scroll event meant for the camera list. (Fixed in MR [!4405](https://gitlab.gnome.org/GNOME/gtk/-/merge_requests/4405/diffs#1b2782cecf88934dfff50b3eadda7a2b036acfe1))

# 
# Feedback is more than welcome
