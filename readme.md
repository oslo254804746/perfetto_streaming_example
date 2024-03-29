## Introduction

This project provides an example of using the `perfetto sdk` to initiate performance testing on the Android system and sends each `TracePacket` to the `PC` end via `socket`.

By default, `perfetto` writes collected data to `/data/misc/perfetto-traces/` on the Android device. The files can be quite large and need to be cleaned up in a timely manner. They also need to be `pulled` to the `PC` for display each time.

This example sends data to the `PC` during the collection process and does not write `TracePacket` to the Android device file. Theoretically, the collection time is unlimited (as long as there is enough space on the `PC`), unless you manually terminate the collection. After the collection is finished, you can directly analyze performance on the `PC` end using `perfetto-ui`.

This example enables metrics such as `ftrace`, `linux.process_stats`, and `android.surfaceflinger.frametimeline`.

According to the official `perfetto` documentation, `perfetto` is pre-installed on devices running `Android 9` and later. However, for `Android 9-10`, you need to manually execute the following command, and devices running `Android 10` and later do not require any action.

`perfetto` also provides `tracebox` support for devices running `Android 9` and below, but after testing, this example code cannot run on devices below `Android 9`.

```shell
# Needed only on Android 9 (P) and 10 (Q) on non-Pixel phones.
adb shell setprop persist.traced.enable 1
```

## Build

The example code is for the `Android` platform and uses the `Android NDK`. For other platforms, you need to remove the `android/log.h` related code in `streaming.cc`.

The following build steps are also for the `Android` platform. Ensure that you have installed the necessary build tools such as `Android NDK/cmake`. If not, you can install them through `Android Studio`.

Build script:

```shell
chmod +x ./build.sh && ./build.sh
```

### Explanation

By default, on `MAC`, the location of the `NDK` installed through `AndroidStudio` is:

```shell
~/Android/sdk/ndk/${version_name}/build/cmake/android.toolchain.cmake
```

And the build command have some variable would look like:

```shell
-DCMAKE_TOOLCHAIN_FILE=~/Android/sdk/ndk/26.1.10909125/build/cmake/android.toolchain.cmake -DANDROID_ABI="arm64-v8a" -DANDROID_PLATFORM=android-18
```
Please Change those variables to yourself

- `CMAKE_TOOLCHAIN_FILE` actual`NDK`location
- `ANDROID_ABI`  actual `Android` arch
- `ANDROID_PLATFORM` actual `Android` device's `SDK` version

## Run

The running steps are divided into the following steps. You can also write your own script, such as `run.sh`.

```shell
#!/usr/bin/env bash

adb shell mkdir /data/local/tmp || 
adb push build/perfetto_streaming_example /data/local/tmp/
adb shell chmod +x /data/local/tmp/perfetto_streaming_example
nohup python demo_recv_perfetto_tracepacket.py &
adb reverse tcp:10086 tcp:10086
adb shell "/data/local/tmp/perfetto_streaming_example -p 10086 -a com.android.chrome"
```

### Pushing Files
If you build through `build.sh`, the output is located at `build/perfetto_streaming_example`. Push it to the device path using `adb`, for example:

```shell
adb shell mkdir "/data/local/tmp" ||
adb push build/perfetto_streaming_example /data/local/tmp/
adb shell chmod +x /data/local/tmp/perfetto_streaming_example
```

### Running the Receiver
The repository provides a `python` receiver `demo_recv_perfetto_tracepacket.py` that can be used directly. It runs on the local machine at port `127.0.0.1:10086`, or you can implement it in another language. The packet format sent by `perfetto_streaming_example` is **4-byte big-endian packet length + packet body**.

```python
    ... # ignored
    while True:
        try:
            length = client_socket.recv(4)
        except:
            continue
        if not length:
            continue
        len_ = struct.unpack("!I", length)[0]
        buf = b""
        while len(buf) < len_:
            try:
                current_buf = client_socket.recv(len_ - len(buf))
                if current_buf:
                    buf += current_buf
                    continue
            except BlockingIOError:
                continue
        print(f"recv trace packet size {len_} -> {len(buf)}")
    ... # ignored
```

### Reverse Port
Since it is necessary to send `TracePacket` via `socket` during the running period, you need to specify the socket address to send to. `perfetto_streaming_example` accepts three arguments:

- The `-a` parameter, which receives the package name of the `app` to be traced, e.g., `com.android.chrome`.
- The `-h` parameter, the host of the socket to send to, with a default value of `127.0.0.1`.
- The `-p` parameter, the port of the socket to send to.

If the `PC` and the Android device can communicate directly, then you do not need to `reverse` the port.

Assuming you are using the repository's `demo_recv_perfetto_tracepacket.py` and your `PC` and Android device are not on the same network segment and connected via `USB`, you need to run the receiver and then execute:

```shell

adb reverse tcp:10086 tcp:10086
```

### Running the Collector
After completing the above steps, you can execute on the device:

```shell
cd /data/local/tmp
./perfetto_streaming_example -h 127.0.0.1 -p 10086 -a com.android.chrome
```

`perfetto_streaming_example` will then send data to the receiver. If you are using the repository's `demo_recv_perfetto_tracepacket.py`, you will see output similar to the following:

```shell
recv trace packet size 33942 -> 33942
recv trace packet size 33917 -> 33917
recv trace packet size 33988 -> 33988
recv trace packet size 23131 -> 23131
recv trace packet size 29351 -> 29351
recv trace packet size 105232 -> 105232
recv trace packet size 54014 -> 54014
recv trace packet size 44356 -> 44356
recv trace packet size 56013 -> 56013
recv trace packet size 45002 -> 45002
recv trace packet size 53590 -> 53590
recv trace packet size 67804 -> 67804
recv trace packet size 51371 -> 51371
recv trace packet size 71027 -> 71027
recv trace packet size 92170 -> 92170
recv trace packet size 56366 -> 56366
recv trace packet size 35206 -> 35206
recv trace packet size 37196 -> 37196
recv trace packet size 104383 -> 104383
```

### Stopping Collection

After running `perfetto_streaming_example`, it will block until you press `CTRL+C(WINDOWS)/COMMAND+C(MAC)`. Otherwise, `perfetto_streaming_example` will continue to run. When you want to stop, press `CTRL+C(WINDOWS)/COMMAND+C(MAC)`, but it may take some time because there is still some data in the buffer that needs to be sent all at once, and the packet body will be quite large.

After the `Android` end stops, you can directly interrupt the receiver. If you are using the repository's example `demo_recv_perfetto_tracepacket.py`, you will see a `test.trace` file.

### Presentation

The above `test.trace` file can be directly displayed in `perfetto-ui`.

