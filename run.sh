#!/usr/bin/env bash
nohup python demo_recv_perfetto_tracepacket.py &
adb shell mkdir /data/local/tmp ||
adb push build/perfetto_streaming_example /data/local/tmp/
adb shell chmod +x /data/local/tmp/perfetto_streaming_example
adb reverse tcp:10086 tcp:10086
adb shell "/data/local/tmp/perfetto_streaming_example -p 10086 -a com.android.chrome"
