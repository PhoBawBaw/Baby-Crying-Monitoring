#!/bin/bash

cd ~/Baby-Crying-Monitoring

if [ ! -d ~/Baby-Crying-Monitoring/bin ]; then
    mkdir ~/bin
fi

if [ -f ~/Baby-Crying-Monitoring/bin/client ] || [ -f ~/Baby-Crying-Monitoring/bin/dht22_reader ]; then
    make clean
fi

make

# 빌드 성공하면 실행
if [ -f ~/Baby-Crying-Monitoring/bin/client ]; then
    ~/Baby-Crying-Monitoring/bin/client
fi

if [ -f ~/Baby-Crying-Monitoring/bin/dht22_reader ]; then
    ~/Baby-Crying-Monitoring/bin/dht22_reader
fi