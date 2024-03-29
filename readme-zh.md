

## 介绍

这个项目提供了一个使用`perfetto sdk` 开启安卓系统性能测试, 并将每个`TracePacket`通过`socket`发送到`PC`端的示例

默认情况下, `perfetto` 会将采集数据写入`Android`设备 `/data/misc/perfetto-traces/` 处, 文件会比较大，需要及时清理，并且每次都需要 `pull`到 `pc`进行展示

这个示例会在采集过程中将数据发送到`PC`端，不会将`TracePacket`写入`Android`设备文件,理论上采集时间无限制(`PC`端空间足够大的话),除非你手动终端采集,采集结束后在`PC`端可以直接通过`perfetto-ui`分析性能

本示例开启了`ftrace`,`linux.process_stats`,`android.surfaceflinger.frametimeline`等指标

根据`perfetto`官方文档, [perfetto-quickstart](https://perfetto.dev/docs/quickstart/android-tracing), `perfetto`在`Android 9`之后的设备都预置了,
但 `Android 9-10`需要手动执行一下命令，`Android 10`之后的设备无需操作

`perfetto` 提供了`tracebox`支持`Android 9`以下的设备，但经过测试, 这个示例代码无法在`Android 9`以下的设备运行



```shell
# Needed only on Android 9 (P) and 10 (Q) on non-Pixel phones.
adb shell setprop persist.traced.enable 1
```



## 构建



示例代码是针对 `Android`平台的, 使用到了`Android NDK`, 其他平台需要去掉`steraming.cc`中 `android/log.h`相关代码

下面的构建步骤也是针对`Android`平台, 请确保已安装`Android NDK/cmake` 等必要的构建工具, 如果未安装可以通过`Android Studio`安装

构建脚本



```shell
chmod +x ./build.sh && ./build.sh
```


### 说明



默认清况下, 在`MAC` 上通过`AndroidStudio` 安装的 `NDK` 位置位于

```shell
~/Android/sdk/ndk/${version_name}/build/cmake/android.toolchain.cmake
```





```shell
-DCMAKE_TOOLCHAIN_FILE=~/Android/sdk/ndk/26.1.10909125/build/cmake/android.toolchain.cmake -DANDROID_ABI="arm64-v8a" -DANDROID_PLATFORM=android-18
```



## 运行

运行步骤分为以下几步，你也可以自行写一个脚本, 例如`run.sh`

```shell

#!/usr/bin/env bash

adb shell mkdir /data/local/tmp || 
adb push build/perfetto_streaming_example /data/local/tmp/
adb shell chmod +x /data/local/tmp/perfetto_streaming_example
nohup python demo_recv_perfetto_tracepacket.py &
adb reverse tcp:10086 tcp:10086
adb shell "/data/local/tmp/perfetto_streaming_example -p 10086 -a com.android.chrome"


```

### 推送文件
如果通过`build.sh` 构建, 产物位于`build/perfetto_streaming_example`, 通过`adb`推送至设备路径, 例如



```shell
adb shell mkdir "/data/local/tmp" ||
adb push build/perfetto_streaming_example /data/local/tmp
adb shell chmod +x /data/local/tmp/perfetto_streaming_example
```



### 运行接收端

仓库提供了一个`python`的接收端`demo_recv_perfetto_tracepacket.py`, 可以直接使用， 他运行在本机`127.0.0.1:10086`端口
或者用其他语言自行实现, `perfetto_streaming_example` 发送的包格式为 **4字节大端包体长度+包体**

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



### reverse 端口

因为需要将`TracePacket`在运行期间通过`socket`发送，因此需要指定需要发送到的`socket`地址, `perfetto_streaming_example`接收三个参数

- `-a` 参数, 接收需要采集的`app`包名, 例如`com.android.chrome`
- `-h` 参数, 发送到的`socket` host, 默认值为 `127.0.0.1`
- `-p` 参数, 发送到的`socket` port


如果`PC`和手机可以直接通信, 那么就可以不需要`reverse`端口

假设你使用的是仓库中的`demo_recv_perfetto_tracepacket.py`, 且`PC`和`Android`设备不在一个网段，通过`USB`连接，那么你需要运行接收端以后执行

```shell

adb reverse tcp:10086 tcp:10086
```



### 运行采集端

上述步骤完成后, 你可以到设备上执行

```shell
cd /data/local/tmp
./perfetto_streaming_example -h 127.0.0.1 -p 10086 -a com.android.chrome
```

`perfetto_streaming_example`就会将数据发送至 接收端, 如果你使用的是仓库中的`demo_recv_perfetto_tracepacket.py`, 那么你会看到类似于下列的输出

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



### 停止采集

当运行`perfetto_streaming_example`后, 会一直阻塞到除非你按下`CTRL+C(WINDOWS)/COMMAND+C(MAC)`，否则`perfetto_streaming_example`会一直运行, 当你想停止的时候, 
按下`CTRL+C(WINDOWS)/COMMAND+C(MAC)` 即可, 但可能需要等待一段时间，因为停止后还有一部分在缓冲区的数据需要一次性发送，包体会比较大

`Android` 端停止后, 你可以直接中断掉接收端, 如果是使用的仓库中的示例`demo_recv_perfetto_tracepacket.py`, 你会看到一个`test.trace`文件



### 展示

上面的`test.trace`文件在`perfetto-ui`中可以直接展示
