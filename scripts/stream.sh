#!/bin/bash

if [ ! -d /app/stream ]; then
    mkdir -p /app/stream
else
    rm -rf /app/stream/*
fi

if [ ! -d /app/audio ]; then
    mkdir -p /app/audio
else
    rm -rf /app/audio/*
fi

if [ ! -d /app/record ]; then
    mkdir -p /app/record
else
   rm -rf /app/record/*
fi

while true; do
    find /app/audio -type f -name "*.wav" -mmin +1 -exec rm {} \;
    sleep 30
done &

ffmpeg -i rtsp://<ip address>:<port>/test \
        -vn -acodec pcm_s16le -ar 44100 -ac 2 -f segment -segment_time 5 -reset_timestamps 1 \
        -strftime 1 "/app/audio/audio_%Y-%m-%d_%H-%M-%S.wav" \
        -c:v copy -c:a copy \
        -hls_time 1 -hls_list_size 1 \
        -hls_flags delete_segments \
        -hls_playlist_type event \
        -fflags nobuffer+genpts \
        -use_wallclock_as_timestamps 1 \
        -muxdelay 0 \
        -f hls /app/stream/stream.m3u8 \
        -f segment -segment_time 60 -reset_timestamps 1 \
        -strftime 1 "/app/record/record_%Y-%m-%d_%H-%M-%S.mp4"
