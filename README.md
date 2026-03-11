# YT-DLP C++ Script
C++ script for downloading music (flac) from youtube or yt music\
It needs `yt-dlp ffmpeg` to work, so you need to install these things on your system PATH before use.

# Usage
1. Select type (currently have only type 1).
2. Enter youtube link (can be youtube music or normal youtube).

# Compile
Currently I tested it only on linux\

### For Linux: 
You need to install `g++ pkg-config libcurl4-openssl-dev libwebp-dev libopencv-dev` on your linux system.\
\
Command to compile in linux bash:
```bash
g++ yt-all_linux.cpp -o yt-all_linux -lcurl -lwebp `pkg-config --cflags --libs opencv4`
```
___
### For Windows: 
You need to install MSYS2 UCRT64 environment for g++.\
\
Install all request component in MSYS2 UCRT64 Terminal:
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
        mingw-w64-ucrt-x86_64-pkg-config \
        mingw-w64-ucrt-x86_64-opencv \
        mingw-w64-ucrt-x86_64-curl \
        mingw-w64-ucrt-x86_64-libwebp
```
Don't forget to add `C:\msys64\ucrt64\bin` to your system PATH.

Command to compile in Windows Terminal (cmd):
```batch
g++ yt-all_linux.cpp -o yt-all_windows -lcurl -lwebp $(pkg-config --cflags --libs opencv4)
```
