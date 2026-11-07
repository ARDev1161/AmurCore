# Getting started

Use the [`feat/rc-api-v2` source branch](https://github.com/ARDev1161/AmurCore/tree/feat/rc-api-v2). Documentation itself is maintained separately and is **not** a buildable source branch.

## Build workstation

The application uses Qt5/C++17. Use the same build flags and dependencies as the [full Qt5 CI workflow](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/.github/workflows/rc-api-v2.yml):

```sh
git clone --branch feat/rc-api-v2 https://github.com/ARDev1161/AmurCore.git
cd AmurCore
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DGRPC_FETCHCONTENT=OFF -DBUILD_APPIMAGE=OFF -DRC_API_V2_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure -R rc_api_v2_
```


## Inspect capabilities

Start the v2-enabled `rc_agent` with a stable robot ID, connect using the existing gRPC transport and open **Robot → Robot Info**. The dialog shows the advertised robot ID, API version, capabilities and velocity axes.

The new `RobotInfo` field is optional; old agents remain supported. The UI is **not yet fleet-safe** and remains single-session until P3.


## Compatibility first

P1 only adds `Robot.Sensors.robot_info = 9`. Existing `Robot.Controls`, map/Nav2 and video paths must continue to work. V2 `Velocity`, task/chat and stream selection are currently **schemas**, not production control paths. Do not enable raw v2 velocity forwarding without TTL, session lease, deadman and watchdog.
