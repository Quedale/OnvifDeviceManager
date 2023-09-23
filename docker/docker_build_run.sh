#!/bin/bash

#Build Dockerfile
docker build -t onvifmgr_container "$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)"

#Allow docker to connect to local DISPLAY to open gtk window 
xhost +Local:*

#Run docker containre
docker run \
    -e DISPLAY=$DISPLAY `#Connect to main display` \
    -v /tmp/.X11-unix/:/tmp/.X11-unix/ `#Provide access to thost X socket file` \
    --rm `#Automaically delete container on exit` \
    --network=host `#Attach to host network to allow UDP discovery` \
    --device "/dev/snd:/dev/snd" `#Mapping speaker device` \
    --name onvifmgr onvifmgr_container


