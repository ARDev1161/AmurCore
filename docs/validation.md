# Validation and migration gates

The canonical [P0–P6 roadmap](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/PLAN.md) defines acceptance. This site describes the **documented P1 checkpoint**, not a claim of full v2 rollout.

| Phase | State | Gate |
| --- | --- | --- |
| P0 | Done | branches and architecture frozen |
| P1 | **In progress** | protobuf mirror, old/new wire, Qt/ROS2, loopback, real robot/manual smoke |
| P2 | Not started | configurable trusted ROS2 endpoints |
| P3 | Not started | independent multi-robot sessions |
| P4 | Not started | tasks/dialogue adapter |
| P5 | Not started | change-only streams and safe teleop |
| P6 | Not started | end-to-end verification |

Historical successful Actions: [AmurCore CI #20](https://github.com/ARDev1161/AmurCore/actions/runs/41262962581) and [rc_agent CI #25](https://github.com/ARDev1161/rc_agent/actions/runs/41263125743). These runs are **not** evidence for all future commits or real hardware.

The [eight-case P1 manual checklist](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/docs/P1_MANUAL_ACCEPTANCE.md) must still verify RobotInfo, legacy clients in both directions, joystick with stop, map/pose/Nav2, video start/stop, idle responsiveness and reconnect. Do not mark P1 DONE on docs or protocol-only CI alone.

## Scope of this documentation CI

It checks documentation branch isolation, Doxygen XML/HTML API generation from a separately checked out source revision, successful Sphinx rendering, Mermaid markup, navigable API pages, and presence of a source SHA in the site title. It does **not** replace the source code's Qt5, ROS2, gRPC or hardware test matrix.

## P1 loopback legacy-after-v2 regression — 2026-11-08

[Source revision bc6d90648d5bd0dfbedb984373524fc1c07857e1](https://github.com/ARDev1161/AmurCore/commit/bc6d90648d5bd0dfbedb984373524fc1c07857e1) extends the **real AmurCore gRPC server** loopback test. After an agent advertises a v2 descriptor via the bidirectional stream, the same service receives a legacy unary sensor snapshot. The test checks that the prior robot ID/capabilities are cleared, the new battery value is retained and the old control reply remains valid.

This is a simulated transport regression, **not a completed P1 manual gate**. [CI run](https://github.com/ARDev1161/AmurCore/actions/runs/42431821920) must pass before this new test is considered verified. The actual robot/VPN/joystick/Nav2/map/GStreamer checks remain outstanding.


## Real AmurCore ↔ rc_agent P1 integration test — 2026-11-08

The independent working branches now run a new **real-peer-grpc-loopback** CI job. Each job checks out the two actual C++ implementations, compares the mirrored protobuf contracts, compiles the production transport peers together, and exercises an ephemeral localhost gRPC connection. The shared scenario, maintained under [AmurCore/tests/real_peer_smoke.cpp](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/tests/real_peer_smoke.cpp), checks legacy control/sensor roundtrips, v2 descriptor and supported axes, legacy-after-v2 reset, fresh-channel reconnect, and workstation mutex availability during callbacks.

The [current CI workflow](https://github.com/ARDev1161/AmurCore/actions/runs/42432805702) must complete successfully before this gate can be marked PASS. No test sends commands to robot actuators. Hardware/VPN/joystick/Nav2/map/GStreamer acceptance remains separate and **not completed**; P1 is still IN PROGRESS.
