# OnvifDeviceManager
Onvif Device Manager for Linux

# Description
The goal of this project is to implement a Onvif Device Manager similar to the windows client, compatible for linux. I'm also working on adding some Profile T capabilities, such as bidirectional audio.

# Working
- Onvif UDP Discovery
- Soap Client for Onvif Device and Media service
- Vew RTSP Stream with backchannel
- Prototype Soundlevel indicator

# TODO
- WIP: Fixed change of camera (pipeline rotation is required because of a "unsupported timestamp reference clock")
- Display Onvif device information
- Display Media information
- Better Look&Feel
- Testing with a variety of camera
- Record video
- JPEG Snapshot
- Push-to-Talk button
- Soundlevel GstOverlay instead of GtkWidget (GtkLevelBar makes UI unresponsive event without event)
- Check for better backchannel audio support. (PCMU@8000 might be Onvif's spec limit)

# Use-case
1. Doorbell-like security camera terminal
2. Babymonitor terminal (I hope to add common baby monitor feature like soundgage and playing music)
3. Cross-room communication terminal

# Note
I have very little spare time to work on any personal project, so this might take a while.
This is my very first C project, so I'm learning as I go. 
 
 
 
 
 
 

# 
# Feedback is more than welcome
