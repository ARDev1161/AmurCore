# P1 manual acceptance: AmurCore ↔ rc_agent

> Scope: one selected robot at a time. This is a **manual hardware/VPN/UI gate**;
> the automated loopback tests do not prove Nav2, 2D map, joystick or video
> operation on real hardware. Record actual results in PLAN.md before closing P1.

## Test build / environment

- AmurCore branch: `feat/rc-api-v2`; build Qt5 application with the commands
  from `.github/workflows/rc-api-v2.yml`. The workstation listens on
  `0.0.0.0:7777` (declared in `amurcore.h`); the local ARP discovery port
  is `11111`, per `amurcore.h`.
- rc_agent branch: `feat/rc-api-v2`, ROS2 Humble. Build and source the
  workspace; the ROS2 executable is `rc_agent` in package `rc_agent`.
- Start with a backup copy of `robot.cfg.in` in **libconfig** syntax (not YAML).
  Set `Amur.API.robot_id = "test-robot-001"`; keep it stable and unique.
  Set `Amur.API.display_name`, `capabilities` and the actual
  `velocity_axes` of this robot.
- On the robot use a full absolute path for `robot_config_path`:
  `ros2 run rc_agent rc_agent --ros-args -p robot_config_path:=/absolute/path/robot.cfg`.
  This parameter is declared on the `rc_agent` ROS2 node in `main.cpp`.
  Network discovery still uses UDP broadcast; ensure discovery works on the
  selected LAN/VPN or record its failure. Do not mistake log "Connected to
  server" (which currently also means the client threads were started) for
  an established gRPC session.

## Manual checks

| Case | Action | Pass condition |
|---|---|---|
| P1-01 Descriptor | Connect v2 agent and open **Robot Info** in AmurCore | Stable robot ID/name, `RC API 2.0`, capabilities and axes match the robot config |
| P1-02 Legacy agent | Connect previous `dev` agent to v2 AmurCore | Existing joystick, map, Nav2 and video controls still work; no fabricated v2 robot descriptor |
| P1-03 Legacy workstation | Connect v2 agent to a separately built `dev` AmurCore | Existing commands and sensor messages still work; unknown `Sensors.robot_info` field does not cause disconnect |
| P1-04 Motion | In a **safe simulation or wheels-off-ground** setup, send a short legacy joystick command and release it | Robot receives expected permitted motion; zero/stop works; no unsupported 6DoF axes are assumed operational |
| P1-05 Map/pose | With `/map` and TF2 publisher active, open map and send a navigation goal to Nav2 | 2D OccupancyGrid, position, waypoint and Nav2 feedback match the actual robot |
| P1-06 Video | With a configured GStreamer sender, choose camera and disconnect/stop stream | Stream plays and stops; no stuck process or continuously sending camera video after disconnect (P5 gating not yet implemented; record the actual limitation) |
| P1-07 Idle | Leave a connected robot stationary for 2 minutes, inspect GUI controls and task/navigation status | GUI remains responsive; no mutex stall during network read, no stuck Nav2 callbacks |
| P1-08 Reconnect | Disconnect and reconnect the same robot, then view Robot Info again | ID/capabilities consistent; state restored from new snapshot, no stale UI data. Full multi-session isolation is a P3 gate |

## Automated checks (not substitutes)

- AmurCore: C++ gRPC loopback server and offscreen Qt5 Robot Info tests, plus
  Python cross-version `dev`↔v2 wire decoding.
- rc_agent: C++ gRPC loopback client against a mocked operator, including
  first stream snapshot from a new channel, and ROS2 Humble `colcon` build/tests.
- CI sources and exact current results are in the two projects' `PLAN.md`.

Capture for each case: branch SHA, test setup, expected/actual behavior, logs,
and PASS/FAIL/SKIP reason. **Do not claim P1 complete based on CI alone.**
