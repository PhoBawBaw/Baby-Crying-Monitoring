#!/bin/bash

if [ ! -d ~/bin ]; then
    mkdir ~/bin
fi

if [ -f ~/bin/Client ]; then
    rm -f ~/bin/Client
fi

g++ -o ~/bin/Client Client.cc `pkg-config —cflags —libs gstreamer-1.0 gstreamer-rtsp-server-1.0`

~/bin/Client