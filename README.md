# yt-dlp_flac_c
C++ script for downloading music (flac) from youtube or yt music

# Usage
1. Select type (currently have only type 1).
2. Enter youtube link (can be youtube music or normal youtube).

# Compile this c++ script
Currently available only on linux (maybe for Windows 10++)\
To compile this, you have to install ``g++, pkg-config, libcurl4-openssl-dev, libwebp-dev, libopencv-dev`` on your linux system\
\
Command to compile:
```bash
g++ yt-all_linux.cpp -o yt-all_linux -lcurl -lwebp `pkg-config --cflags --libs opencv4`
