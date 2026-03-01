# yt-dlp_flac_c
C++ script for download music (flac)

to build this program
  in linux: you need to install libopencv-dev libcurl4-openssl-dev libwebp-dev on your system
            use this command to build
        --> g++ yt-all_linux.cpp -o yt-all_linux -lcurl -lwebp `pkg-config --cflags --libs opencv4`
