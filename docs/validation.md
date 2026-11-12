# Validation and migration gates

The canonical [P0–P6 roadmap](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/PLAN.md) defines acceptance. This site describes the **documented P1 checkpoint**, not a claim of full v2 rollout.

| Phase | State | Gate |
| --- | --- | --- |
| P0 | Done | branches and architecture frozen |
| P1 | **In progress** | protobuf mirror, old/new wire, Qt/ROS2, loopback, real robot/manual smoke |
| P2 | **In progress** (authorized overlap; P1 manual pending) | trusted local ROS2 endpoint profiles, two robot profiles, real Nav2 result/cancellation tests |
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


### Real-peer acceptance result (CI, 2026-11-08)

The cross-repository test passed on the exact source revision [`8da7cc1303fb7e6687e7f889792bc260d771a309`](https://github.com/ARDev1161/AmurCore/commit/8da7cc1303fb7e6687e7f889792bc260d771a309): [workflow #42432935800](https://github.com/ARDev1161/AmurCore/actions/runs/42432935800) **SUCCESS**. Both repositories now compile and run the actual workstation gRPC server with the actual gateway gRPC client in CI. This is a headless P1 transport acceptance check, **not** P1-01 through P1-08 real-robot/UI/Nav2/GStreamer acceptance; P1 remains in progress.


## Real operator disconnect (P1) — 2026-11-08

The `real-peer-grpc-loopback` job now exercises the real `AmurCore` server shutdown while the real `rc_agent` client is connected. A one-shot unary attempt must return a failed gRPC status within four seconds rather than hang, preserving the last received controls. This is a loopback integration check, **not** the physical/VPN/Nav2/GStreamer/manual acceptance gate.

[Source revision **956f2b970885**](https://github.com/ARDev1161/AmurCore/commit/956f2b970885c65664067f8a5a84eea2dc080a91) — [CI workflow](https://github.com/ARDev1161/AmurCore/actions/runs/42435043821) (check status for this exact SHA).


## P1: real-peer stream stop (2026-11-08)

The [real-peer integration scenario](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/tests/real_peer_smoke.cpp) now also opens a real `rc_agent` streaming RPC to the actual `AmurCore` gRPC service, verifies a snapshot arrived, then calls `stopStream()` and requires the stream to terminate with cancellation within three seconds. The workstation telemetry callback must remain responsive throughout.

[Source revision `fd68254c572c`](https://github.com/ARDev1161/AmurCore/commit/fd68254c572c20961d80be6ae089e5457feb38b2), [current CI](https://github.com/ARDev1161/AmurCore/actions/runs/42441869752) — check completion before marking PASS. This does not exercise the actual robot or replace the manual P1-01 through P1-08 gates.


## P1 coherent map snapshots over real gRPC peers — 2026-11-08

The headless [real-peer test](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/tests/real_peer_smoke.cpp) now allocates **independent** agent-side and operator-side map objects. It verifies that an initial `GetMapResponse` with `map_seq=71` and robot pose arrives through actual gRPC transport, then that a changed snapshot with `map_seq=72` is received, and finally that `stopStream()` cancels the real map RPC. This prevents accidental success through a shared in-memory pointer.

[Source 0526a1e1c48a](https://github.com/ARDev1161/AmurCore/commit/0526a1e1c48a53d8a41b76e1810fabf223ad66a1) · [CI 42444276933](https://github.com/ARDev1161/AmurCore/actions/runs/42444276933). CI outcome must be checked for the exact source SHA before marking PASS. No real robot, Qt map rendering, physical motion, VPN, Nav2 or GStreamer is exercised here; P1-01..08 are still required.


## P1 legacy PoseStream and ZoneStream real-peer regression — 2026-11-08

The headless [real-peer integration test](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/tests/real_peer_smoke.cpp) now independently verifies the legacy pose and zone streams after stopping the map stream. With separate agent/operator map objects, it checks pose x=3.75 followed by x=4.25 and `CANCELLED` stream shutdown; it then checks zone ID 1001 with name `alpha` followed by `beta` on zone sequence 802, and another `CANCELLED` shutdown. It also asserts that pose transport did not replace map sequence 72 and zone transport did not replace pose or map data.

[Working source 7c8e1990aeac](https://github.com/ARDev1161/AmurCore/commit/7c8e1990aeacae6a23b4f7d8328e1d5608bc884a) / [CI 42445927699](https://github.com/ARDev1161/AmurCore/actions/runs/42445927699) (verify final result before claiming PASS). This is headless software transport evidence, not physical GUI/Nav2/VPN/GStreamer acceptance. P1 remains IN PROGRESS.


## P1 cancellation verification and current peer changes — 2026-11-08

The actual AmurCore/rc_agent headless gRPC peer test passed on AmurCore source revision [`60a6dd9e`](https://github.com/ARDev1161/AmurCore/commit/60a6dd9e7f3261deb5a73ef1a539af7fac0a2f64), [Actions #42450768529](https://github.com/ARDev1161/AmurCore/actions/runs/42450768529), **3/3 jobs SUCCESS**. The paired rc_agent test at [`b05bab6d`](https://github.com/ARDev1161/rc_agent/commit/b05bab6d1340d3de4ed9bd1bd63ef460f327b27b) passed [Actions #42450762717](https://github.com/ARDev1161/rc_agent/actions/runs/42450762717), **5/5 jobs SUCCESS**. These verify shutdown of a blocked unary RPC and a stopped client's rejection of new unary calls, not a physical motor stop.

Subsequent rc_agent commits [`453be21f`](https://github.com/ARDev1161/rc_agent/commit/453be21f643797a8450399c806898a9f7454c37f), [`4cb1faa9`](https://github.com/ARDev1161/rc_agent/commit/4cb1faa9e0cc77e8ebbece37e44357413683bf40), [`1a0f0a50`](https://github.com/ARDev1161/rc_agent/commit/1a0f0a50930db89a5e0117bfedb74a717d8df129) and [`fc28ea70`](https://github.com/ARDev1161/rc_agent/commit/fc28ea7083c9af2efb7d3c50e954470e999c9b6c) address mixed unary/stream shutdown and stream retry backoff cancellation. Their CI is **NOT VERIFIED** here; historical green runs do not validate those commits. No AmurCore runtime change was needed. Manual P1-01–P1-08 remain **NOT RUN**; P1 IN PROGRESS and P2 TODO.


## P1 video pipeline metadata legacy/v2 real-peer regression — 2026-11-08

The [real AmurCore ↔ rc_agent gRPC integration](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/tests/real_peer_smoke.cpp) now asserts the legacy `Sensors.video_stream.pipeline` is sent on initial unary exchange, an updated pipeline arrives alongside v2 RobotInfo without erasing the ID, a fresh reconnect snapshot clears the old pipeline, and an updated pipeline travels on legacy DataStreamExchange.

[Source revision `348f4cdde1ce`](https://github.com/ARDev1161/AmurCore/commit/348f4cdde1ce7ea0d24e1e1468e24a18f5902aa1) — [CI 42503577387](https://github.com/ARDev1161/AmurCore/actions/runs/42503577387) (verify complete results for exact source SHA). This validates *metadata transport only*, not live GStreamer capture/playback. P1 hardware/VPN/video/GUI/Nav2 gates remain NOT RUN; no P2 work.

## P2 auxiliary velocity profile binding — 2026-11-13

The gateway binds the legacy `cmd_vel`, `cmd_vel_smoothed` and `cmd_vel_unsafety` selectors only to trusted local ROS2 endpoints from the libconfig robot profile. `cmd_vel_smoothed_topic` and `cmd_vel_nav_topic` are now exposed in the gateway's `Amur.API.endpoints` settings and passed to the ROS2 adapter; omitted values retain legacy defaults. Two independent config files and invalid-type/empty values are covered by gateway C++ GTests. This does **not** implement v2 velocity authorization.

[Functional rc_agent test revision `15327dc0`](https://github.com/ARDev1161/rc_agent/commit/15327dc0eebd7b36294c742177d19f8607bb0692) · [source CI #43327905089](https://github.com/ARDev1161/rc_agent/actions/runs/43327905089) (verify completion for this revision). P1 software/manual and physical gates remain open.
