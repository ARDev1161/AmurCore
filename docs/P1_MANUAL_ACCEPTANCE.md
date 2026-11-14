# P1 manual acceptance: AmurCore ↔ rc_agent

> Owner override (2026-11-13): v2-only acceptance. Legacy dev↔v2 pairing and [-255,255] velocity scaling are obsolete. Do not enable remote velocity until TTL/session/lease/deadman/watchdog are enforced.
>
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
| P1-02 v2-only handshake | Connect v2 agent to v2 AmurCore | v2 handshake and capabilities work; legacy RPC is not required or accepted |
| P1-03 v2-only wire | Exchange v2 commands and telemetry between feature builds | Commands and telemetry use only v2 messages; old dev pairing is not an acceptance requirement |
| P1-04 Motion | In a **safe simulation or wheels-off-ground** setup, send a short v2 SI-unit velocity command through the authorized control path and release it | Robot receives expected permitted motion; zero/stop works; no unsupported 6DoF axes are assumed operational |
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

## TurtleBot3 / Gazebo Sim P1 integration lane (agreed 2026-11-09)

The operator already has a container that starts TurtleBot3 in **Gazebo Sim**
(the ROS2 Humble recommended pairing is Gazebo Fortress). Reuse that container;
do not prescribe a new Compose file, Gazebo installation, or hard-coded
container name. This lane can run without the physical robot being repaired.

**Network scope:** the operator explicitly waived *repeating VPN-specific
acceptance* if the connection/discovery/session model is unchanged. Before
marking that item N/A, compare the current feature implementation with the
legacy `dev` connection path (UDP discovery, gRPC transport, reconnect).
If anything changed, record the difference and test it separately. Localhost
success does not prove VPN traversal.

**Setup evidence (capture before testing):**
- Container/image identifier and startup command (operator-provided), ROS distro,
  `gz sim --version`, `ros2 pkg list | grep -E 'turtlebot3|nav2|ros_gz'`.
- `ros2 topic list -t`, `ros2 action list -t`, `ros2 node list`, and
  `ros2 topic info /cmd_vel -v`; do not assume the topic name or message
  type until these commands confirm it. If `/map`, TF, Nav2, or a camera are
  missing, record SKIP and reason; do not invent sensor publishers.
- Both feature branch commit SHAs, actual `robot.cfg` path, configured
  robot_id/capabilities/velocity_axes, AmurCore address and whether UDP
  discovery crosses the container network boundary.

**Run and record these checks on the running simulator:**

| Gate | Action | Evidence and pass condition |
|---|---|---|
| P1-01 | Start v2 agent with a unique nonempty robot_id and connect AmurCore; open Robot Info | Screenshot/log confirms exact configured ID, display name, API 2.0, capabilities and axes |
| P1-02/03 | Run both cross-version *application* pairings with separately built `dev` binaries | Real legacy controls and telemetry survive both directions; unknown RobotInfo ignored; protobuf-only CI is not sufficient |
| P1-04 | With Nav2 idle, send a short legacy joystick command and release it | Capture `/cmd_vel` with message type and timestamps; simulated robot moves and stops after release; never infer a physical safety guarantee |
| P1-05 | With map, TF and Nav2 actually available, display map/pose and send a reachable Nav2 goal | Capture map/pose, Nav2 action feedback and result; goal accepted alone is not success |
| P1-06 | Use a separately configured GStreamer test sender if TurtleBot3 has no camera | Video starts and stops on disconnect; note that inactive-robot video gating belongs to P5 |
| P1-07 | Keep simulator stationary for two minutes with an open GUI | Controls remain responsive, no blocked gRPC/Qt callbacks |
| P1-08 | Restart rc_agent (same ID), reconnect and inspect Robot Info/map | Fresh snapshot restored, no stale capability/pose/map; multi-robot isolation remains P3 |

For each row record **PASS / FAIL / SKIP**, exact commands, relevant logs,
expected versus observed behavior, and SHAs in PLAN.md. Do not mark an
unperformed test PASS. Simulator success may satisfy the P1 *software
integration* evidence, but physical joystick, motor/watchdog and real-camera
behavior remain separately **NOT RUN** until hardware is available. P1 is
still IN PROGRESS until the owner reviews the applicable gate evidence; P2
must not be started automatically.
