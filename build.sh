#!/usr/bin/env bash

rm -rf build
mkdir build && cd build
cmake -G Ninja -S ../ -DCMAKE_TOOLCHAIN_FILE=~/Android/sdk/ndk/26.1.10909125/build/cmake/android.toolchain.cmake -DANDROID_ABI="arm64-v8a" -DANDROID_PLATFORM=android-30 && cmake --build .
adb shell mkdir /data/local/tmp && adb push build/perfetto_streaming_example /data/local/tmp
