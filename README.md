# OnvifDeviceManager
Onvif Device Manager for Linux

![Application Capture](AppCapture.png?raw=true "OnvifDeviceMgr Linux")

# Description
The goal of this project is to implement a Onvif Device Manager similar to the windows client, compatible for linux. I'm also working on adding some Profile T capabilities, such as bidirectional audio.

# Working
- Onvif WS-Discovery (gsoap)
- Soap Client for Onvif Device and Media service
- Vew RTSP Stream with backchannel
- Prototype Soundlevel indicator
    -- The CairOverlay causes a segmentation fault on older version of Cairo
    -- Looking into OverlayComposition
- WS-Security (Currently using hardcoded placeholder credentials "admin/admin")

# TODO
- WIP : WS-Security Authentication support.
    - Credential Input
    - Credential Storage 
- EventQueue : Interupt pending events when needed
- Display Onvif device information
- Display Media information
- Better Look&Feel
- Testing with a variety of camera
- Record video
- JPEG Snapshot
- Push-to-Talk button
- Check for better backchannel audio support. (PCMU@8000 might be Onvif's spec limit)

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
### Bootstrap using system's Gstreamer libraries
```
./bootstrap.sh
```
 
### [Optional] Bootstrap and build Gstreamer from source
```
./bootstrap.sh --with-gstreamer
```
### [Optional] Build GTK from source
This is usefull when compiling using a older distribution
```
./build_gui_deps.sh
```
### Conifigure project
This project supports out-of-tree build to keep the source directory clean.
```
mkdir build && cd build
../configure --prefix=$(pwd)/dist
```

### Compile WS-Discovery Library
This is usefull when compiling using a older distribution
```
make discolib
make install-discolib
```


### Compile Onvif Soap Library
```
make onvifsoaplib
make install-onvifsoaplib
```

### Compile and install GUI App
```
make
make install
```
# Note
I have very little spare time to work on any personal project, so this might take a while.
This is my very first C project, so I'm learning as I go. 
 
 

# 
# Feedback is more than welcome
