# OnvifDeviceManager
Onvif Device Manager for Linux

![Application Capture](images/AppCapture.png?raw=true "OnvifDeviceMgr Linux")

# Description
The goal of this project is to implement a Onvif Device Manager similar to the windows client, compatible for linux. I'm also working on adding some Profile T capabilities, such as bidirectional audio.

# Working
- Onvif WS-Discovery and WS-Security (gsoap)
- Soap Client for Onvif Device and Media service
- Vew RTSP Stream with backchannel (Push-to-talk)
- Prototype Soundlevel indicator
- Support system and static libraries. (static recommended)
- Support Multiple ONVIF Profiles
- Tested H264, H265 and MJPEG stream up to 4K
- Logging framework
- HTTP authentication challenge (Basic, Digest and NTLM)
- EventQueue for background tasks
- Encrypted Credential Storage 

# TODO
- EventQueue GUI (TaskManager)
- GObject rollout
- Edit Onvif device identification
- Display Onvif network information
- Display Media information
- Add PTZ controls
- Testing with a variety of camera
- Record video
- JPEG Snapshot
- Support casting (Chromecast, Onvif NVD, Mircast, etc..)
- And a lot more...

# Use-case
1. Doorbell-like security camera terminal
2. Babymonitor terminal (I hope to add common baby monitor feature like playing music)
3. Cross-room communication terminal

# Tested with
- [rpos](https://github.com/Quedale/rpos)
- [v4l2onvif](https://github.com/mpromonet/v4l2onvif)
- Merit LILIN (PSR5024EX30 & SR7424/8)
- Reolink (RLC-820A)
- Imou (Ranger 2C)
- D-Link (Tapo C200)
- [Yi-Hack Allwinner V2](https://github.com/roleoroleo/yi-hack-Allwinner-v2) (Pro 2K Home)

# How to build - Option 1 : Flatpak
### Install flatpak-builder
```
# For Debian/Ubuntu
sudo apt install flatpak-builder

# For Fedora
sudo dnf install flatpak-builder

# For Arch Linux
sudo pacman -S flatpak-builder
```

### Add flathub repository
```
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

### Build container
```
cd deployment
flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir io.github.quedale.onvifmgr.yml
```

### Run container
At this point you should have appropriate Desktop menu available, but you can use the following command line:
```
flatpak run io.github.quedale.onvifmgr
```


# How to build - Option 2 : Native
### Clone repository
```
git clone https://github.com/Quedale/OnvifDeviceManager.git
cd OnvifDeviceManager
```
### Autogen, Configure, Download and build dependencies
autogen.sh will attempt download and build missing dependencies.   
**[Mandatory]** The following package dependencies are mandatory and are not yet automatically built:
```
sudo apt install git
sudo apt install libgtk-3-dev
sudo apt install make
sudo apt install g++
```
**[Optional]** The following package are optional, but will reduce the runtime of autogen.sh if installed.
```
sudo apt install python3-pip
python3 -m pip install meson
python3 -m pip install ninja
python3 -m pip install cmake
sudo apt install pkg-config
sudo apt install bison 
sudo apt install flex 
sudo apt install libtool
sudo apt install libssl-dev
sudo apt install zlib1g-dev
sudo apt install libasound2-dev
sudo apt install libgudev-1.0-dev
sudo apt install libx11-xcb-dev
sudo apt install gettext
sudo apt install libpulse-dev
sudo apt install nasm
sudo apt install libntlm0-dev
```
If your system already has gstreamer pre-installed, I strongly recommend using `--enable-latest` to download the latest gstreamer release supported.   
Note that autogen will automatically call "./configure".
```
./autogen.sh --prefix=$(pwd)/dist --enable-latest
make -j$(nproc)
```
At this point, you should be able to execute the application without installing it on the system

```
./onvifmgr
```

### Install
#### debian
Under debian, I recommend using the package manager:
```
make deb
sudo dpkg -i onvifmgr_0.0.deb
```
#### other
For the time being, other distros can use the following
```
sudo make install
```

### Uninstall
#### debian
```
sudo dpkg -r onvifmgr
```
#### other
```
sudo make uninstall
```

#
# Note
I have very little spare time to work on any personal project, so this might take a while.
This is my very first C project, so I'm learning as I go. 

# 
# Attributions
[<img width="20" height="20" src="https://www.flaticon.com/media/dist/min/img/favicon.ico">](https://www.flaticon.com/) [FlatIcon](https://www.flaticon.com/)
<br/><br/>
If I missed any attributions, feel free to report it so that I can add it.<br/>
This is just a hobby project and I didn't mean to steal anything.

# 
# Feedback is more than welcome
