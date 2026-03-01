# yt-dlp_flac_c
C++ script for downloading .flac music file from youtube or yt music.<br/>
This project was made for learning C++, so it will have some bugs or problem that I didn't fix.

URL from youtube must be like this
```
https://music.youtube.com/watch?v=XXXXXXXXXXX
```
or
```
https://www.youtube.com/watch?v=XXXXXXXXXXX
```
# Build Executable Program
To build this program in linux, You need to install `g++ libopencv-dev libcurl4-openssl-dev libwebp-dev` on your system.

Build with GNU C++ Compiler in bash:
```bash
g++ yt-all_linux.cpp -o yt-all_linux -lcurl -lwebp `pkg-config --cflags --libs opencv4`
```
