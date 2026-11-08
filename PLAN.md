# RC API v2 — cross-repository implementation plan

> **Source of truth for all AI contributors.** Read this plan and `AGENTS.md` before making changes. Update status and handoff notes in the *same commit* as functional work. Do not claim a phase complete without passing its acceptance checks.
>
> Coordinated branches: [AmurCore](https://github.com/ARDev1161/AmurCore/tree/feat/rc-api-v2) · [rc_agent](https://github.com/ARDev1161/rc_agent/tree/feat/rc-api-v2) · [robot_orchestrator](https://github.com/ARDev1161/robot_orchestrator/tree/feat/rc-api-v2) · [ros2_rknn_llm](https://github.com/ARDev1161/ros2_rknn_llm/tree/feat/rc-api-v2). The canonical architectural decisions are in [AmurCore/PLAN.md](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/PLAN.md); each repository keeps a locally actionable copy of the plan.

## Product contract (approved requirements)
- **AmurCore is an operator workstation, NOT a robot controller, LLM planner, PDDL engine or fleet scheduler.** Operator can select one of many connected robots and use joystick/camera/map; give direct Nav2 goals; submit text/audio to an orchestrator; observe autonomous tasks.
- **rc_agent is a configurable protocol gateway** between the workstation and *pre-existing* robot ROS 2 topics/services/actions or local gRPC endpoints. Never put inverse kinematics, gait, balancing, motor drivers, path planning, task scheduling, or LLM processing in rc_agent.
- Control hierarchy: physical/motor layer outside rc_agent; direct 6-DOF velocity and Cartesian end-effector pose; supervisory navigation; semi-autonomous language tasks; robot-initiated autonomous tasks. Use supported-axis declarations; do NOT add per-joint operator control.
- Assignment: explicitly selected robot -> that robot; otherwise ask external fleet allocator/OpenRMF. AmurCore never decides priority/preemption/robot allocation internally.
- **Busy teleop interception:** display active task and exactly three choices: back (unchanged), pause (resume after teleop release), cancel (do not auto-resume). Never fake pause by silently canceling Nav2; implementing backend pause/resume is a distinct acceptance gate.
- Tasks are hierarchical at any depth. Show one-line summary + expandable child states; report LLM/PlanSys2/Nav2 stages with real task/parent IDs. “accepted by planner” is NOT “execution succeeded”.
- Asynchronous bidirectional conversation; AmurCore sends text or recorded audio unchanged via rc_agent; robot can initiate prompts days later; correlate conversation and task. Robot owns history and logs; AmurCore can view/export.
- Each robot has its own 2D OccupancyGrid/pose/map; keep current map/NavDialog, waypoints/patrol and GStreamer video. Optional fused 2D maps are provided by external systems, not in rc_agent; 3D deferred.
- Robot sends a configured **capability/endpoint profile** (only abstract capabilities to AmurCore; do not disclose ROS endpoint wiring unless needed for diagnostics). No dynamic UI framework: use existing widgets, simple optional action/parameter controls.
- **Traffic:** heartbeat periodic; on connect/reconnect send coherent snapshot; thereafter publish battery/pose/health/tasks only on meaningful changes (thresholds/debounce configurable), never compare whole maps on CPU. Video is OFF for non-selected robots and GStreamer sender should stop or be gated. Inactive robots remain connected; low-rate health if necessary. Do not poll unchanged data forever.
- VPN works; no cloud broker or public-internet infrastructure in scope for v2. No assumption that all ROS actions/services can be introspected without an adapter.
- C++17, Qt5 and ROS 2; no dependency on proprietary tooling. Existing production endpoints and serialized wire fields are a compatibility boundary: additive protocol first, cutover only with tests.

## Ground-truth code pointers
- AmurCore `network/protobuf/{robot,base,navigation,map,video}.proto`, `network/server.cpp`, `amurcore.cpp`, `logic/{movements,navdialog,mapwidget}.cpp`.
- rc_agent `main.cpp`, `modules/{basecontrol,nav2,map}/`, `network/client.cpp`, `robot.cfg.in`, `params/basecontrol.yaml`.
- LLM `src/ros2_rknn_llm_node.cpp` and `src/planner_client.cpp`; planner `robot_planner/src/goal_gateway.cpp`. The planner gateway currently reports submitted/accepted/rejected; it does NOT track full execution.
- Existing AmurCore `AGENTS.md` must be preserved and extended, not discarded.
- `robot_orchestrator` is a **superproject with Git submodules**; modifications in `ros2_rknn_llm` do NOT automatically update the superproject's pinned SHA. Update the submodule pointer in a controlled later commit and do not make a main/master deployment claim without it.

## Work phases and gates (do not skip)
| Phase | State | Work (owning repo) | Acceptance / evidence |
|---|---|---|---|
| P0 Baseline + coordination | DONE | feature branches; AGENTS.md + PLAN.md in all affected repos; inventory of current flow | document base refs, no default branch changes |
| P1 Additive API skeleton | IN PROGRESS | synced minimal proto across AmurCore + rc_agent; agent config -> RobotInfo; first receipt/inspection on workstation; tests | same proto byte-for-byte, legacy `Controls`/`Sensors` fields untouched, `protoc` generates; old clients continue; config defaults safe |
| P2 Robot-side endpoint bindings | TODO | configurable ROS2 cmd_vel (Twist/TwistStamped), odom/map/TF/Nav2 action names, optional local gRPC adapters with allowlists; config validation | two robot profiles, different endpoint names, no code edits; unknown types rejected; action cancellation status correct |
| P3 Multi-robot isolation | TODO | AmurCore per-connection sessions/state, selection/UI routing, per-robot maps+task+video, connection lifecycle | two concurrent agent simulators cannot cross-control or mix map/sensors; selection routes correctly; disconnect releases velocity authority |
| P4 Task + dialogue transport | TODO | correlate task/parent/stage ID, status snapshot/event, text/audio forwarding, asynchronous robot-originated chat + history/log requests | planner accept != completed; expandable stages, late prompt, disconnect/reconnect; bounded audio chunks |
| P5 Efficient streams + teleop preemption | TODO | snapshot + changed state, rate/hysteresis; independent map updates, video enable on selected robot; explicit pause/resume/cancel with backend acknowledgements | idle video network=0; no stale velocity after disconnect; no task resumes after cancel |
| P6 Cross-repo verification | TODO | integration tests (2 robots, Nav2 mock, LLM/PlanSys2 mock, GStreamer), docs, migration instructions and draft PRs | build, unit + contract tests green; observed status trace; compatibility matrix; no unreviewed merges |

## Required contract details before wiring P1/P2
- Identify `robot_id` stably from config; reject duplicate live IDs (no silent overwrite), session per authenticated stream, protocol major/minor; reconnect increments session generation.
- Velocity: linear XYZ, angular XYZ in explicit coordinate frame and **SI units** on v2 wire; existing joystick’s -255..255 legacy scale must be translated only at an explicit boundary; supported axes sent by robot. Command validity/deadman/watchdog mandatory before enabling v2 teleop. Keep local controller safety authoritative.
- Task: unique `task_id`, optional parent ID, source, typed lifecycle; accepted/queued/running/succeeded/failed/canceled/paused distinct. Reject duplicate command effects; no claim exactly-once.
- For ROS topic publish, `dispatched` means emitted, NOT physical completion. True success only from action/result/state feedback.
- Streaming: changes by semantic value with numeric threshold, event ID/sequence for replay; no expensive occupancy-grid comparison.
- Video: robot supplies ready-to-use GStreamer connection pipeline or URL; workstation chooses camera; default OFF when robot not selected.
- Security: allowlist configured endpoints; never accept arbitrary remote-supplied ROS topic, gRPC service/method, scripts, shell commands, or plugin paths; use local config authority.
- Pause/resume semantics and OpenRMF allocation are outside rc_agent; if upstream executor cannot pause, explicitly report unsupported and disable Pause button.

## Implementation rules for every contributor
1. Before editing: read **this file, `AGENTS.md`**, relevant source code and other-repo contract. Check outstanding `IN PROGRESS` items; take a small well-defined scope. Do not reimplement features already present.
2. One repository/phase per focused commit; no unrequested merge to `dev`/`master`. Keep existing ROS2 Nav2 and GStreamer functional. Never delete legacy messages, wire field numbers, or endpoints in P1.
3. Update this file's phase checklist and **Handoff** with exact changes, commit SHA or URL, tests actually executed, blockers, and next task. If change touches shared proto, sync exact definitions to BOTH repositories and prove equality.
4. Tests required: `protoc` for modified proto (when available); `cmake`/colcon on matching target OS; regression for legacy Nav2/map/video; deterministic unit tests. If dependencies absent, record `NOT RUN` with reason, never claim green.
5. Do not invent support for a ROS service/action type: register a tested adapter or fail closed. No unbounded binary audio in gRPC streams. Control must expire safely.
6. Before any change to the user-approved product contract, update an **Open decisions** entry and obtain explicit confirmation rather than silently introducing a framework.
7. GitFlow: `main/master` stable, `dev` integration where present, `feat/rc-api-v2` for this work, optional `fix/*` focused bugfixes. Conventional commits. Document owner/repo, base and deps.

## Open decisions and constraints
- Dedicated shared `rc_protocol` repository vs mirrored proto: defer; **mirror identically for P1** to avoid build-system/repo proliferation.
- Exact authentication/TLS implementation: not in P1, but do not expose public network interfaces insecurely.
- Task state replay/retention policy: define before P4; robot owns logs and canonical task state.
- ROS2 interface types and parameter mapping for nonstandard robots: P2 built-in adapters first; no dynamic script injection.
- OpenRMF integration later through external adapter, not rebuilt inside AmurCore.

## Handoff (append on each change)
- 2026-11-01: Cross-repository `feat/rc-api-v2` branches created from AmurCore:dev, rc_agent:dev, robot_orchestrator:master, ros2_rknn_llm:master. Product requirements captured above. P1 starting. No production branch changed.

### AmurCore current lane
- First: preserve existing `navigation.proto`, `map.proto`, `video.proto`, `NavDialog`, `MapWidget`, and GStreamer receiver.
- P1 work: add protobuf additive messages, receive robot capability snapshot safely. **Do NOT** retrofit a dynamic UI engine. P3: replace global `Controls/Sensors/map` with per-stream contexts *before* accepting multiple robots simultaneously.
- Tests: compile Qt5 app and generated protobuf; manual smoke: joystick->legacy drive, map position/waypoints, video selection.


## P1 progress checkpoint — 2026-11-01

**Implemented in this coordinated branch**
- [x] Same `network/protobuf/rc_api_v2.proto` in AmurCore and rc_agent; additive `Robot.Sensors.robot_info = 9`; legacy Controls, Nav2, Map and Video field numbers untouched.
- [x] rc_agent: `robot.cfg.in` optional `Amur.API.{robot_id,display_name,capabilities,velocity_axes}`; list reader with type validation; metadata populated *before* starting the network stream; a blank robot ID defaults to legacy mode; ROS2 param `robot_config_path`.
- [x] AmurCore: existing Robot Info dialog shows advertised capability list and axes; receives under existing sensor mutex. **Still ONE shared session**, not fleet-safe; multi-robot work remains P3.
- [x] rc_agent: two new config parser GTests authored (valid list / invalid mixed list) but **NOT EXECUTED**.
- [x] ros2_rknn_llm: planner `kAccepted` now publishes JSON `status: accepted`, not `succeeded`; README clarified. Actual executor completion reporting remains a P4 deliverable.
- [x] robot_orchestrator: feature-branch gitlink updated to commit `aa7c4b28bd2bc0462cfe52daa9c795d257abbb24` of ros2_rknn_llm (do not confuse with submodule master).

**Validation completed:** repository source inspections; checked BOTH new proto texts byte-for-byte identical; BOTH robot.proto texts byte-for-byte identical; verified legacy wire tag positions, agent descriptor assignment, AmurCore reader, LLM status mapping, valid C++ test fixture string escaping, and superproject gitlink. All 10 checks PASSED (static inspection only).

**NOT RUN / blockers:** `protoc`, `grpc_cpp_plugin`, ROS2/`colcon`, Qt5, libconfig++ headers and full build/test dependencies are not installed in the available execution environment. Neither protobuf code generation nor C++ compilation, GTests, Nav2/GStreamer smoke tests, or multi-robot sessions have been executed; do not claim P1 accepted/green.

**Next work (in order):**
1. On a development machine with dependencies: `protoc -I network/protobuf --descriptor_set_out=/tmp/rc-v2.pb network/protobuf/rc_api_v2.proto network/protobuf/robot.proto` on both checkouts; `colcon build --packages-select rc_agent` + `colcon test --packages-select rc_agent` (inside sourced ROS 2 workspace); Qt5 Debug CMake build of AmurCore. Fix issues before advancing.
2. Functional P1 smoke: set unique robot ID in agent config; connect; open Robot Info; verify capability/axis labels; connect old agent against new AmurCore and new agent against old AmurCore; check Nav2 map/joystick/GStreamer regression.
3. Add focused RobotInfo validation and reconnect snapshot test; only then mark P1 DONE and start P2 with configurable hardcoded odom/map/TF/Nav2 action endpoints.
4. Security and safety before v2 velocity enablement: don't route raw v2 velocities to motors; add TTL, session binding, deadman and watchdog in P5.

**Known limitations:** adding `RobotInfo` to the old shared `Sensors` stream is a transitional bridge only; it is not safe for two simultaneous robots until P3 isolation. `Velocity`, task/chat and stream-selection messages are **schemas only**, not wired into runtime. Empty robot ID intentionally means no announcement, even if capabilities are configured. P1 remains IN PROGRESS pending builds and compatibility smoke tests.


## P1 CI and regression checkpoint — 2026-11-01
**New automatic tests** (on `feat/rc-api-v2` pushes and PRs):
- AmurCore + rc_agent `tests/test_proto_contract.py`: exact legacy `Controls`/`Sensors` field numbering, additive `robot_info` field 9, legacy Nav2/map/video services, protobuf roundtrips, TaskStatus acceptance semantics, Chat oneof and velocity TTL. C++ `tests/proto_cpp_smoke.cpp` compiles generated protobuf classes and verifies real serialization. GitHub Actions compares the mirrored `robot.proto` and `rc_api_v2.proto` byte-for-byte.
- rc_agent existing + new `ConfigStringList` GTests compiled/executed against libconfig++. CTest typo corrected to point to `rc_agent_unit_tests`.
- AmurCore `qt-app-build` compiles the complete Qt5 application using system packages; rc_agent `ros2-humble-build` compiles/tests the full ROS2 Humble package using `ros:humble-ros-base`. These are separate from quick contract jobs, not substitutes.
- ros2_rknn_llm `tests/test_planner_status_contract.py` asserts accepted != completed, cancellation/rejection reporting and output JSON fields. This is a source-contract check without RKLLM hardware, not an end-to-end LLM runtime test.
- robot_orchestrator workflow verifies the pinned `ros2_rknn_llm` gitlink tracks the feature branch and runs the pinned regression tests.
**Actual green runs (historical SHAs):** [AmurCore protocol #4](https://github.com/ARDev1161/AmurCore/actions/runs/41259526950), [rc_agent protocol + config #4](https://github.com/ARDev1161/rc_agent/actions/runs/41259529672), [ros2_rknn_llm status #1](https://github.com/ARDev1161/ros2_rknn_llm/actions/runs/41259502714), [robot_orchestrator gitlink #3](https://github.com/ARDev1161/robot_orchestrator/actions/runs/41260175233). Future commits require a fresh check of HEAD, these are not sufficient to mark P1 complete.
**Issues discovered by real CI, fixed on feature branches:** old Ubuntu protoc 3.12 needs proto3-optional flag; system Protobuf/gRPC need CMake module/library fallbacks; rc_agent grpcClient incorrectly tried to lock shared_ptr<mutex> instead of *mutex; downstream MapNode and BaseControl/RobotNavigator integration had inconsistent mutex ownership; VideoSourceRelay::convertAndScale was static but uses instance format settings; AmurCore NavDialog relied on `absl::string_view` even with older protobuf. These were hidden by protocol-only checks.
**Full-build verification:** still IN PROGRESS / NOT YET GREEN as of this checkpoint. Look at latest GitHub Actions run and fix all remaining compiler/CTest errors; do not assert production or hardware behavior without smoke testing. All changes stay on `feat/rc-api-v2`, P1 remains IN PROGRESS until full build, compatibility and manual Nav2/map/video checks.


## Verified P1 automated CI checkpoint — 2026-11-01 (after actual Actions execution)

**Automated checks that passed** (live GitHub Actions, not just local inspection):
- **AmurCore** full Qt5 application build and protobuf C++/Python wire-contract jobs: [run #10, both jobs success](https://github.com/ARDev1161/AmurCore/actions/runs/41260332212), feature commit `463a059b139d444c0da8607a060b213706d95e52`. Build included RobotInfoDialog + generated v2 API.
- **rc_agent** complete ROS 2 Humble `colcon build` and `colcon test`, config GTests, protobuf C++/Python compatibility: [run #16, all three jobs success](https://github.com/ARDev1161/rc_agent/actions/runs/41260131515), feature commit `bfa6bb9d1c317b789fc18acf077dde76b38b2c4e`.
- **ros2_rknn_llm** source contract for acceptance vs completion (not RKLLM hardware integration): [run #1, success](https://github.com/ARDev1161/ros2_rknn_llm/actions/runs/41259502714).
- **robot_orchestrator** pinned feature LLM submodule and source contract: [run #4, success](https://github.com/ARDev1161/robot_orchestrator/actions/runs/41260442446). This can require a new gitlink commit after updates to child documentation; verify HEAD pin matches child HEAD.

**Real bugs discovered and fixed via full build**: mutex shared-pointer dereferencing, MapNode constructor ownership, nonstatic video resize state, proto3 optional flag, CMake Protobuf/gRPC system package compatibility, Qt generated protobuf include path, Abseil enum string_view misuse, CTest typo. Regressions now caught by CI. All modified production source files included in the successful build commits above.

**P1 status remains IN PROGRESS (manual gates)**: automatic compilation/testing is green, but hardware/VPN/manual cross-version sessions remain unverified. Still required: (a) configure nonempty stable ID on real rc_agent and inspect Robot Info in running AmurCore, (b) old/new combinations and Nav2 motion/map/waypoints/GStreamer smoke, (c) safety and multi-session isolation phases P3/P5. Do not claim these pass until executed.

**What next AI must do**: use CI jobs as mandatory merge gates, investigate any red latest run, verify feature HEAD with checks before PR. Then run robot/GUI manual acceptance checks for P1 and record evidence; proceed to P2 only after gates. `dev`/`master` are unchanged.


## P1 loopback / compatibility expansion — 2026-11-01

New headless integration tests complement the earlier protobuf-only and full-build checks:
- AmurCore `tests/grpc_server_smoke.cpp`: launches the **actual** `grpcServer` on an ephemeral localhost port and exercises legacy unary DataExchange, v2 RobotInfo and velocity axes on the same sensor wire, bidirectional control exchange, callback, 2D map streaming and UI mutex availability during an idle stream. The implementation in `network/server.cpp` now reads and writes **outside** the GUI mutex, copying only sensor/control snapshots under the mutex. This prevents operator UI blockage on network reads.
- AmurCore `tests/test_legacy_wire_interop.py`: builds *unmodified* `dev` protobuf Python modules separately and runs a second interpreter to prove old→new and new→old wire parsing, including old readers ignoring optional `robot_info`. Automated CI [run #16](https://github.com/ARDev1161/AmurCore/actions/runs/41262624885) passed the cross-version contract and full application build.
- AmurCore `tests/robot_info_dialog_smoke.cpp`: offscreen Qt5 UI test checks descriptor display, second robot replacing the first, and fallback when no v2 descriptor. The `RC_API_V2_TESTING` CMake option keeps tests optional in normal builds; CI enables it.
- rc_agent `tests/grpc_client_smoke.cpp`: real `grpcClient` against a loopback mocked operator, validates unary and streaming handoffs, maintains ROS mutex availability during a stalled reply and verifies the initial snapshot on a newly created channel. The transport no longer requires `GRPC_CHANNEL_READY` before first send: gRPC's Write/Read handle CONNECTING and reconnection correctly. All four jobs in [rc_agent CI #22](https://github.com/ARDev1161/rc_agent/actions/runs/41262775490) **passed** (ROS2 Humble colcon build/test, config GTest, Python/C++ protobuf compatibility, real client loopback).
- Manual pass criteria and command examples: [AmurCore/docs/P1_MANUAL_ACCEPTANCE.md](https://github.com/ARDev1161/AmurCore/blob/feat/rc-api-v2/docs/P1_MANUAL_ACCEPTANCE.md).

**State:** P1 remains IN PROGRESS pending real-robot/VPN, video, Nav2 and old↔new live application smoke. Hardware tests have NOT been run here. Do not mark P2 active or P1 done without manual evidence. P5 changed-only telemetry requires an explicit separation of control response latency from telemetry emission to avoid delaying joystick commands; do not blindly suppress identical sensor requests in the legacy lockstep RPC.
**Handoff:** inspect the most recent AmurCore `RC API v2 contracts` run (the one containing offscreen Qt test and strengthened mutex regression); if red, open the failed job log and fix. Then run manual checklist and record results. Do not merge `dev`/`master`.


## P1 current automated acceptance — verified on GitHub Actions, 2026-11-01

| Repository | Last tested production-code revision | Actual CI result |
|---|---|---|
| AmurCore | `65f47b8025ff1e5b3f9c8eb90b13587ff27a7f32` | [#20](https://github.com/ARDev1161/AmurCore/actions/runs/41262962581): **SUCCESS**, full Qt5 build + C++ gRPC server/idle-mutex + offscreen RobotInfo Qt + independent dev/v2 protobuf interop |
| rc_agent | `fae3ebddc810e76b1532d3db65512f7ac4807ef6` | [#25](https://github.com/ARDev1161/rc_agent/actions/runs/41263125743): **SUCCESS**, all four jobs: ROS2 Humble build/colcon tests, config GTests, C++ gRPC loopback client, Python/C++ protobuf contract |

**Production fixes** included in the successful commits: AmurCore releases the GUI mutex during network read/write; rc_agent copies sensor/control snapshots under its mutex but performs blocking RPC I/O outside it. All client streams can start when gRPC channel is CONNECTING; readiness is determined by actual Write/Read, preventing an empty initial snapshot. Invalid libconfig scalar types (robot ID/name and numeric/bool settings) return failure rather than an uncaught `SettingTypeException`. A dedicated GTest verifies no output mutation on invalid input.

**Tests**: legacy↔v2 unknown-field parsing, command/sensor/map streaming, callback, idle transport liveness, first message on fresh channel, UI descriptor replacement/fallback, malformed scalar/list config. Automated tests run in CI and all passed as indicated above; no test was run against the user's real robot, physical joystick, live Nav2/Map/TF2, VPN or GStreamer sender.

**P1 state: IN PROGRESS, hardware/manual acceptance outstanding.** Execute `docs/P1_MANUAL_ACCEPTANCE.md` (AmurCore repo) and capture logs and actual pass/fail before moving to P2. `dev`/`master` remain untouched. Do not misinterpret this automated gate as complete fleet, real telemetry change-only gating (P5), or live operator task control.


## P1 regression hardening — 2026-11-08
- P1 remains **IN PROGRESS**; P2 is not authorized until the manual acceptance checklist is completed.
- AmurCore test commit `1bcc98b2757a62d40b56f46665c4c7d0bdeec7b4`: offscreen RobotInfo regression now checks that a descriptor without stable robot_id cannot leak the previous robot's capabilities, name, or axes.
- rc_agent test commit `ce4394199df2d40a927508d49f4bd3562cff0010`: gRPC client loopback now checks that both unary and first stream snapshot deliver capability and axis along with robot ID; mock observation is mutex-protected to avoid a data race.
- Shared proto and legacy wire definitions **unchanged**. The above are test-only changes, not proof of real robot behavior.
- CI: check latest feature HEAD after this PLAN commit and record actual job outcomes; prior green runs are historical, not evidence for these new commits. Local Qt5/ROS2 builds and physical/VPN/Nav2/GStreamer tests: **NOT RUN** in this tool environment.
- Next: review feature HEAD CI jobs, fix any regression; then execute P1-01 through P1-08 with the physical robot or approved equivalent and record evidence. Do not mark P1 DONE without manual acceptance.


## P1 gRPC legacy-after-v2 regression — 2026-11-08
- [AmurCore CI #42429632698](https://github.com/ARDev1161/AmurCore/actions/runs/42429632698) passed full Qt5 and protobuf jobs for the earlier RobotInfo Qt test revision `1bcc98b`; `PLAN.md` alone subsequently advanced HEAD.
- Added a headless real `grpcServer` loopback regression that first receives v2 RobotInfo through the existing data stream, then accepts a legacy unary sensor snapshot on the same service. Asserts that old v2 metadata is completely removed, legacy battery data is applied, and legacy control reply is unchanged. No production or proto changes.
- Paired rc_agent fix: [recursive mock capture / legacy reset](https://github.com/ARDev1161/rc_agent/commit/da2b081dbe25a4bf8c560cfa3db7eb0f0ac18252). Shared proto stays byte-for-byte mirrored.
- **CI pending for this test commit**; check latest HEAD's `protobuf-compatibility` and `qt-app-build` jobs before declaring automatic acceptance. Hardware/VPN/joystick/Nav2/map/GStreamer P1-01..08 remain **NOT RUN** and P1 stays **IN PROGRESS**.


## P1 CI verified after descriptor regression — 2026-11-08
- Working source/test commit `bc6d90648d5bd0dfbedb984373524fc1c07857e1`: [RC API v2 CI run #42431821920](https://github.com/ARDev1161/AmurCore/actions/runs/42431821920) **SUCCESS** at this exact test revision. Full Qt5 application / offscreen & gRPC server tests, protobuf compatibility and old/new wire decoding passed.
- Paired peer tested source/test commit `da2b081dbe25a4bf8c560cfa3db7eb0f0ac18252`; the common `robot.proto` and `rc_api_v2.proto` remained unchanged and mirrored.
- `documentation` revision `fe6e2ae533949da09d12cc18c66af375adbfac7b`: [Sphinx/Doxygen/Pages run #42431878233](https://github.com/ARDev1161/AmurCore/actions/runs/42431878233) **SUCCESS**, generated API source checkout pinned at tested commit during build. Documentation remains separate from production branches.
- This is **CI-only evidence**, not P1 hardware acceptance. P1 remains **IN PROGRESS** with P1-01..08 live robot/VPN/joystick/map/TF2/Nav2/GStreamer checks outstanding. No P2 implementation is authorized before manual gate; next safe action is to record real smoke results when robot is available, or add only genuinely useful deterministic P1 regressions. Do not misinterpret an untested `PLAN.md`-only successor commit as a source-code CI pass.


## P1 cross-repo real gRPC peer test (2026-11-08)
- Added tests/real_peer_smoke.cpp: deterministic test compiling REAL AmurCore grpcServer with REAL rc_agent grpcClient (checked out independently by CI). Covers unary legacy, additive v2 RobotInfo capabilities, legacy-after-v2 clearing, new-channel reconnect snapshot and GUI mutex availability in the callback.
- The test does not perform ROS2 navigation, joystick, video, or physical robot checks; P1 remains IN PROGRESS. The paired CI workflow must be wired and validated before this test is claimed as passed. Shared wire schemas and production code are unchanged.


## P1 real-peer CI gate (2026-11-08)
- New real-peer-grpc-loopback job compiles BOTH real AmurCore server and rc_agent client together using mirrored proto, runs the shared AmurCore scenario on ephemeral localhost with legacy/v2/legacy/reconnect snapshots, and fails on lost controls/descriptor or callback mutex blocking.
- Both repositories have the same test job; this branch checks out the other peer at feat/rc-api-v2. No production endpoints, proto, ROS2 bindings or motor control changed. CI is PENDING until both jobs finish; hardware acceptance P1-01..08 remains NOT RUN, P1 IN PROGRESS.


## Verified P1 cross-repository real-peer CI — 2026-11-08
- Tested source+CI revision `8da7cc1303fb7e6687e7f889792bc260d771a309`: [RC API v2 workflow #42432935800](https://github.com/ARDev1161/AmurCore/actions/runs/42432935800) **SUCCESS** on that exact SHA. All 3 jobs pass: full Qt5 app/CTest, protobuf + dev↔v2 wire compatibility, and real AmurCore gRPC server ↔ rc_agent client integration.
- Paired real peer source/CI revision `41db27715aa2e453742fb712b548b7bcdfe51dd2`. New integration job verifies legacy controls, v2 robot capabilities/axes over the unchanged RPC, legacy-after-v2 clearing, fresh-channel reconnect and nonblocking sensor callbacks. Initial CI attempts exposed build-time header-name collisions and were corrected explicitly; no production code or proto changed.
- Updated documentation in the separate documentation branch: [Pages run #42432865763](https://github.com/ARDev1161/AmurCore/actions/runs/42432865763) SUCCESS. Documentation will refresh its displayed source revision to this tested source SHA.
- **Acceptance**: P1 remains IN PROGRESS. These are headless in-process loopback tests, not hardware/VPN/real joystick/Nav2/map/GStreamer acceptance. Execute P1-01 through P1-08 and capture actual results before starting P2. No merges to dev/master.


## P1 real-peer disconnect contract — 2026-11-08
- Added to AmurCore/tests/real_peer_smoke.cpp: after legacy/v2/reconnect success, shutdown the **real** AmurCore gRPC server and verify the **real** rc_agent one-shot unary call returns a non-OK status within 4 seconds, preserving the last valid controls. Paired rc_agent source hardens retry policy and 3s unary deadlines (see rc_agent feat/rc-api-v2), without altering any protobuf field.
- These tests remain pure headless localhost P1 software checks. CI pending; P1 remains IN PROGRESS until hardware/VPN/joystick/Nav2/map/GStreamer acceptance, no P2 work or default branch merges.


## Verified P1 transport disconnect and retry regression — 2026-11-08
- Tested source SHA `956f2b970885c65664067f8a5a84eea2dc080a91` — [GitHub Actions #42435043821](https://github.com/ARDev1161/AmurCore/actions/runs/42435043821) **SUCCESS** at exactly this revision. All 3 jobs: complete Qt5 build/CTest, Protobuf+dev↔v2 wire, real AmurCore/rc_agent gRPC integration with explicit server disconnect.
- Tested peer `29cda7bf4a390bb5f672feacdbcd4f70d880588a`; both projects compile real peer implementations over unchanged legacy gRPC and mirrored Protobuf.
- Docs source changes only in separate `documentation` branch, SHA `0d7a16986fce38396232062ad8021f6a330f0ac5`, [Pages CI #42435175700](https://github.com/ARDev1161/AmurCore/actions/runs/42435175700) **pending at handoff**; check deployment and fix if red. The documentation tree must remain isolated and must not be merged to source branches.
- P1 still **IN PROGRESS**. Headless CI does not validate actual operator GUI workflow, physical joystick, hardware/VPN, Nav2/TF/map/video, safety watchdog or end-to-end lifecycle. Manual P1-01..08 remain **NOT RUN**; no P2 or default branch changes. Stream `stopStream()` still cannot cancel a blocking gRPC Read; atomic flag prevents retry only after the read unblocks. Do not claim fully resilient teleop until the P5 safety gates are implemented.


### Verified docs and code CI handoff — 2026-11-08
- Source/test SHA `956f2b970885c65664067f8a5a84eea2dc080a91`: [RC API v2 CI #42435043821](https://github.com/ARDev1161/AmurCore/actions/runs/42435043821) **SUCCESS** (all 3 jobs).
- Docs-only SHA `0d7a16986fce38396232062ad8021f6a330f0ac5`: [GitHub Pages CI #42435175700](https://github.com/ARDev1161/AmurCore/actions/runs/42435175700) **SUCCESS** (Sphinx/Doxygen isolation + Pages deploy).
- These success claims are scoped to exact source and docs revisions; plan-only handoff commit does not rerun source CI because workflows filter paths. P1 manual hardware/GUI/VPN/Nav2/GStreamer gates remain **NOT RUN**. Next permitted work: additional targeted P1 reliability tests or manual P1 acceptance; do not implement P2 until approved gate passes. Do not merge documentation or source feature branches.


### P1 paired rc_agent failed-stream regression — 2026-11-08
- rc_agent added a regression asserting failed one-shot streaming RPC does not overwrite valid legacy controls: [test commit ef600cfa](https://github.com/ARDev1161/rc_agent/commit/ef600cfaa17e8ac9a34de13795d148ccc2481385), [paired PLAN handoff 4a2ec9c](https://github.com/ARDev1161/rc_agent/commit/4a2ec9c74adfe0673d20207e6b95639b38a8f276). AmurCore runtime/test source and mirrored proto unchanged.
- New rc_agent feature-head CI: NOT VERIFIED (connector commit status endpoint returned no statuses or PR-triggered runs); do not infer success. Previous tested AmurCore source SHA `956f2b970885c65664067f8a5a84eea2dc080a91` remains historical CI evidence only. P1 IN PROGRESS; hardware/VPN/joystick/Nav2/map/video and live old↔new P1-01..08 remain NOT RUN. P2 not started.


## P1 paired real-peer stream shutdown — 2026-11-08
- Extended `tests/real_peer_smoke.cpp` to run the real rc_agent `DataStreamExchange()` against the real AmurCore gRPC service, observe its initial streaming snapshot, call `stopStream()`, assert the stream exits within 3s with CANCELLED status, and recheck nonblocking workstation callback. This exercises the rc_agent active ClientContext cancellation change without modifying AmurCore production code.
- rc_agent implementation lives only in its `feat/rc-api-v2` branch, and both jobs compile it via separate checkout. Protobuf and Qt/ROS2 behavior unchanged. Current CI PENDING; hardware/VPN/joystick/Nav2/GStreamer P1-01..08 remain NOT RUN, P1 IN PROGRESS; no P2.


### Verified P1 active-stream cancellation on current code — 2026-11-08
- Tested source/test SHA `fd68254c572c20961d80be6ae089e5457feb38b2`: [GitHub Actions run 42441869752](https://github.com/ARDev1161/AmurCore/actions/runs/42441869752) **SUCCESS** for all 3 jobs (including real `AmurCore` ↔ `rc_agent` gRPC and full Qt5/CTest and mirrored protobuf tests). The cross-repo peer checked out was at `598c6da8efd42bd0de120c7947f2c2a776677019`.
- Docs-only `documentation` SHA `1addd3f4192e7b369aa628caf9a651181f46f5af`: [Sphinx/Doxygen/Pages run 42441934353](https://github.com/ARDev1161/AmurCore/actions/runs/42441934353) **SUCCESS**; no documentation sources were copied to working C++ branches.
- `stopStream()` now cancels blocking gRPC stream I/O; this is lifecycle cancellation, NOT proof of robot physical stop or v2 velocity lease/deadman/watchdog. A stopped client is terminal and must be replaced to reconnect. Current branch HEAD may differ by this PLAN-only handoff update; do not misrepresent the tested SHA.
- Phase **P1 IN PROGRESS**. Hardware/VPN/operator GUI/joystick/Nav2/map/video/old↔new live tests P1-01..08 remain **NOT RUN**. No P2 development or merge to dev/master permitted until gates; next meaningful task is physical/simulator manual acceptance when available, or a targeted P1 blocker regression.


### P1 concurrent streaming cancellation regression — 2026-11-08
- rc_agent test revision [5bb547e05e24df03610da0938964294278e41b6b](https://github.com/ARDev1161/rc_agent/commit/5bb547e05e24df03610da0938964294278e41b6b) adds two simultaneous blocked DataStreamExchange calls on the same real grpcClient, asserts stopStream() cancels both contexts, repeated stop is idempotent, CANCELLED statuses are returned, and previously valid controls remain unchanged. No runtime or shared proto changes; AmurCore real-peer source unchanged.
- CI: PENDING verification for the new test SHA; earlier successful runs are not evidence for this test. Hardware/VPN/joystick/Nav2/map/GStreamer and live dev↔v2 manual P1-01..08 remain NOT RUN. P1 IN PROGRESS; P2 not authorized. Next: inspect current rc_agent Actions job grpc-client-loopback, repair any failure, and update docs source SHA if needed.


## P1 real map gRPC transport acceptance (CI) — 2026-11-08
- Extended `tests/real_peer_smoke.cpp` with **separate operator and agent map storage** (critical: old shared pointers could falsely pass integration tests). The real rc_agent `MapStream()` must send full initial `GetMapResponse` map sequence 71 + pose, then updated sequence 72 + pose to the real AmurCore service, after which `stopStream()` must cancel the map RPC cleanly. The original legacy sensor/controls, streaming and reconnect tests remain intact.
- Paired `rc_agent` source change copies map sequence and payload coherently under `muMap` and performs blocking gRPC Write/Read outside the mutex. No changes to proto, Nav2 or GStreamer. CI for this commit pending; hardware/VPN/joystick/map display/TF2/Nav2/video P1-01..08 still NOT RUN. P1 IN PROGRESS, no P2.


## P1 verified coherent map transport integration — 2026-11-08
- Source/test SHA `0526a1e1c48a53d8a41b76e1810fabf223ad66a1` [CI #42444276933](https://github.com/ARDev1161/AmurCore/actions/runs/42444276933) **SUCCESS** for all 3 jobs at that exact tested SHA; includes real `AmurCore` ↔ `rc_agent` gRPC test exchanging independent map instances with initial sequence 71 + pose and updated sequence 72 + pose, then canceling the map stream. Full Qt5/CTest and Protobuf dev↔v2 interop also passed.
- Complementary peer source/test SHA: `403d499735f5b996ee86011c0ee459569d05de11`. No shared protobuf modification; `robot.proto` and `rc_api_v2.proto` remain mirrored. Existing legacy Nav2/video controls are preserved, not validated on real hardware.
- Documentation in separate `documentation` branch at `f6ef51fe85bea248a2d612b2024810a3894f8ad0` [workflow #42444346158](https://github.com/ARDev1161/AmurCore/actions/runs/42444346158): Sphinx/Doxygen isolation **PASS**; **Pages deployment QUEUED at time of handoff**, do not claim complete deployment until it finishes. CI/Pages must be rechecked after GitHub completes pending deployment.
- **P1 IN PROGRESS**. Manual P1-01 through P1-08 on actual robot/VPN/GUI/joystick/Nav2/map display/GStreamer/old↔new binaries remain NOT RUN. P2 TODO, no dev/master merge. Next meaningful P1 action: review any pending red CI/deploy, then conduct manual acceptance or add targeted simulation only if it closes a specific uncovered failure.


### Completed documentation deployment for coherent map CI — 2026-11-08
- [Documentation branch revision `f6ef51fe85bea248a2d612b2024810a3894f8ad0`](https://github.com/ARDev1161/AmurCore/commit/f6ef51fe85bea248a2d612b2024810a3894f8ad0): [Sphinx/Doxygen + Pages run 42444346158](https://github.com/ARDev1161/AmurCore/actions/runs/42444346158) **SUCCESS** including deployment, updated from tested source `0526a1e1c48a53d8a41b76e1810fabf223ad66a1`. This supersedes the earlier *QUEUED* handoff in this file.
- Working-code [CI run 42444276933](https://github.com/ARDev1161/AmurCore/actions/runs/42444276933) **SUCCESS** on `0526a1e1c48a53d8a41b76e1810fabf223ad66a1`. No changes in this handoff to production code or mirrored protobuf. P1 remains IN PROGRESS until P1-01..08 live acceptance; P2 remains TODO.


## P1 real-peer legacy pose/zone stream isolation — 2026-11-08
- Extended AmurCore/tests/real_peer_smoke.cpp after the existing independent-map test. Once MapStream is explicitly stopped, launch **real** rc_agent PoseStream and verify initial pose x=3.75 then changed pose x=4.25, cancel and assert CANCELLED. Assert map_seq 72 is preserved.
- Then launch **real** rc_agent ZoneStream against AmurCore server with zone_seq 801, id 1001/name alpha; update to zone_seq 802/name beta; verify each through the real gRPC boundary, cancel and assert CANCELLED. Assert map_seq and pose are untouched by the zone stream. No shared map pointer or alternative stream is active; this detects missing or stale legacy streaming updates.
- Pair with rc_agent source change making ZoneStream zone_seq/zone_map snapshot atomic and removing RPC creation under muMap for Pose/Zone. Protobuf and production AmurCore code unchanged. **CI PENDING** at authorship for both feature commits; P1 manual P1-01..08 still NOT RUN, P1 IN PROGRESS. No P2.


## P1 verified separate PoseStream and ZoneStream loopback — 2026-11-08
- Tested source/test commit `7c8e1990aeacae6a23b4f7d8328e1d5608bc884a`: [GitHub Actions #42445927699](https://github.com/ARDev1161/AmurCore/actions/runs/42445927699) **SUCCESS** for all 3 jobs, including true cross-repository `AmurCore` ↔ `rc_agent` gRPC, mirrored proto and complete Qt5 build/offscreen regressions.
- Peer checked out from `feat/rc-api-v2` during cross-repo CI: `f2ad035d2139a6a0b534ef2b1ea07f2aec5c5c6e`. Under independent map storage, the real peer scenario proved MapStream initial and changed snapshots, then separate PoseStream initial/changed pose and ZoneStream initial/changed zone (map stream stopped), plus CANCELLED stream shutdown and preservation of map/pose fields. No changes to mirrored protobuf or legacy RPC IDs.
- Documentation-only branch tested SHA `a9d702108c3370ff28c6cd3bb467ac2986dc7001`: [GitHub Pages CI #42445965943](https://github.com/ARDev1161/AmurCore/actions/runs/42445965943) **SUCCESS** (strict Sphinx/Doxygen isolation and Pages publish). This PLAN-only successor commit does not need to change production/test source and must not be misrepresented as the tested code SHA.
- **P1 still IN PROGRESS**. These tests use headless localhost, not the user's live robot, VPN, Qt navigation workflow, physical joystick, Nav2/TF2/OccupancyGrid visualization, live old↔new binaries or GStreamer; manual P1-01..P1-08 remain **NOT RUN**. P2 still TODO, dev/master not touched.
- Next meaningful action: inspect pending feature-head CI on any future code changes and run P1 manual acceptance on an available simulation/robot; only extend automated P1 coverage to close an identified compatibility or deadlock failure. No artificial claims of complete v2 migration.


## P1 stopped unary real-peer regression — 2026-11-08
- Test [60a6dd9e](https://github.com/ARDev1161/AmurCore/commit/60a6dd9e7f3261deb5a73ef1a539af7fac0a2f64) extends the actual AmurCore server ↔ rc_agent client loopback: after stopStream, the peer must reject unary DataExchange with CANCELLED promptly and leave the last controls intact. Paired rc_agent production fix [981a7fbe](https://github.com/ARDev1161/rc_agent/commit/981a7fbe14b9f18d38ef4dbba28477f3db1abe89), regression test [b05bab6d](https://github.com/ARDev1161/rc_agent/commit/b05bab6d1340d3de4ed9bd1bd63ef460f327b27b).
- GitHub Actions result for this AmurCore test commit NOT VERIFIED at authoring; latest earlier tested [#42445927699](https://github.com/ARDev1161/AmurCore/actions/runs/42445927699) succeeded on different SHA 7c8e1990. rc_agent [#42450762717](https://github.com/ARDev1161/rc_agent/actions/runs/42450762717) in progress at handoff. Local Qt5/ROS2 builds NOT RUN. No proto, API contract, Nav2/video, or production AmurCore changes.
- P1 IN PROGRESS, manual P1-01..08 (hardware/VPN/joystick/Nav2/map/video/live legacy compatibility) NOT RUN. P2 blocked by acceptance. Next: inspect current feature-head CI, fix failures, refresh separate documentation source SHA only after tested code is confirmed; no dev/master merge.


### P1 unary cancellation CI verified — 2026-11-08
- Feature source/test revision `60a6dd9e7f3261deb5a73ef1a539af7fac0a2f64` passed [Actions #42450768529](https://github.com/ARDev1161/AmurCore/actions/runs/42450768529): 3/3 jobs SUCCESS (Qt5 build/CTest, protobuf + dev/v2 interoperability, actual AmurCore↔rc_agent C++ gRPC peer). This supersedes the earlier PENDING handoff. The current branch has a subsequent PLAN-only commit; do not conflate its SHA with tested source.
- Paired rc_agent [#42450762717](https://github.com/ARDev1161/rc_agent/actions/runs/42450762717): 5/5 jobs SUCCESS including blocked unary cancellation. These checks do not prove live hardware or physical stop.
- P1 IN PROGRESS, P1-01..P1-08 manual gates NOT RUN; P2 TODO. No source/proto/default branch changes in this CI verification handoff. Next: targeted simulator/live acceptance; documentation remains isolated in `documentation`.


### P1 coordinated mixed unary/stream shutdown handoff — 2026-11-08
- Paired rc_agent production [453be21f](https://github.com/ARDev1161/rc_agent/commit/453be21f643797a8450399c806898a9f7454c37f) rejects late streaming controls after stopStream; [test 4cb1faa9](https://github.com/ARDev1161/rc_agent/commit/4cb1faa9e0cc77e8ebbece37e44357413683bf40) checks concurrent unary+stream shutdown on one client. No AmurCore production/test source, protobuf, or UI changes in this handoff.
- New rc_agent CI **NOT VERIFIED**; prior paired Actions #42450768529 and #42450762717 succeeded for earlier tested revisions, not this change. Local build/test **NOT RUN** (GitHub DNS unavailable). P1 IN PROGRESS, hardware/VPN/joystick/Nav2/map/GStreamer/live legacy P1-01..08 NOT RUN; P2 TODO. Next: verify current rc_agent Actions and source checkout, then update isolated documentation when confirmed.


### P1 rc_agent stream retry shutdown coordination — 2026-11-08
- Peer rc_agent fix [1a0f0a50](https://github.com/ARDev1161/rc_agent/commit/1a0f0a50930db89a5e0117bfedb74a717d8df129) returns CANCELLED if stopStream interrupts streaming retry backoff; regression [fc28ea70](https://github.com/ARDev1161/rc_agent/commit/fc28ea7083c9af2efb7d3c50e954470e999c9b6c) checks a real mock gRPC failure followed by shutdown. No AmurCore source or protobuf changes. New rc_agent CI NOT VERIFIED at handoff; do not treat earlier green runs as this revision. P1 IN PROGRESS; P1-01..08 NOT RUN; P2 TODO. Next: verify peer CI, run simulator/manual gates.


### P1 optional battery peer handoff (2026-11-08)
- rc_agent added opt-in ROS2 `sensor_msgs/msg/BatteryState` subscriber configured via `Amur.API.battery_topic` (libconfig), writing under the gRPC Sensors mutex to existing optional `Sensors.batteryState=5`; no proto changes. Source commits [`5ab8cf1`](https://github.com/ARDev1161/rc_agent/commit/5ab8cf13b1276831759b78c29f39ac3f2d2c266c), [`49e7f85`](https://github.com/ARDev1161/rc_agent/commit/49e7f858f0cbdb6db6a634245f58b74ec531320c), test [`c157a22`](https://github.com/ARDev1161/rc_agent/commit/c157a224dd507e1947048935cfe8b4626a6b806a). AmurCore production unchanged; UI battery absent/stale/NaN behavior still requires implementation/review. New rc_agent CI NOT VERIFIED, local build NOT RUN. P1 IN PROGRESS, P1-01..08 NOT RUN, P2 TODO.


### P1 optional battery display (2026-11-08)
- AmurCore source [`402d7e50`](https://github.com/ARDev1161/AmurCore/commit/402d7e50c2105760f866dd9b0f5dbfe3e5e71c3a) / [`4e893071`](https://github.com/ARDev1161/AmurCore/commit/4e893071730985d03a2e11aeb930614c4ebe183c): optional battery indicator in existing status bar; read `Sensors.batteryState` under gRPC mutex, hide if field absent or `present=false`, render finite 0..1 percentage and charging state, otherwise 'Battery: unknown'. UI updated on existing throttled sensor callback, no protobuf changes. Peer rc_agent has opt-in `Amur.API.battery_topic` ROS2 subscriber. Caution: ROS publisher loss/staleness timeout and disconnect-specific freshness not yet implemented; do not claim those are solved. CI/Qt build NOT VERIFIED, manual P1-01..08 NOT RUN; P1 IN PROGRESS, P2 TODO. Next: run current SHA Qt/ROS2 CI, fix compile/test failures, implement staleness handling and UI tests.


### P1 BatteryState freshness (2026-11-08)
- rc_agent [`ddac0d40`](https://github.com/ARDev1161/rc_agent/commit/ddac0d40a7fa27cb8216d5b6f4d38551d2d1d88a) and [`1ff18cac`](https://github.com/ARDev1161/rc_agent/commit/1ff18cacbe21ae54927f2203458b27cc177e8a05) add a steady-clock 5-second BatteryState receive timeout. ROS subscription and 250-ms timer use the default mutually exclusive callback group; on timeout the agent clears the optional `Sensors.batteryState` under the same mutex used for gRPC snapshots. Fresh data repopulates it. AmurCore already hides absent battery on its sensor-update event. No protobuf changes. WARNING: current CI/build and live topic timeout test NOT VERIFIED; UI disconnect behavior independent of stale ROS battery remains to be verified. P1 IN PROGRESS, manual P1-01..08 NOT RUN, P2 TODO.


### P1 disconnect battery guard (2026-11-08)
- AmurCore [`d47fa494`](https://github.com/ARDev1161/AmurCore/commit/d47fa49489f9280265be7dbe2b142dba510c651b) hides cached battery when robot list becomes empty; [`3db5ef21`](https://github.com/ARDev1161/AmurCore/commit/3db5ef21d22f11b3d93e4377f2b04a8222315ebc) additionally prevents stale sensor-refresh callback from re-showing it while no robots are known. This does not implement P3 per-session selection. rc_agent [`b372cb81`](https://github.com/ARDev1161/rc_agent/commit/b372cb817e8c90037ff0cc7ca5987f9c931cd606) documents battery freshness behavior in public Doxygen comments. No shared proto change. Current-head CI and Qt/ROS2 compile NOT VERIFIED in this handoff; hardware P1-01..08 NOT RUN. P1 IN PROGRESS; next: inspect current-head Actions, compile/test, add deterministic stale timeout regression, then manually check disconnect/reconnect.


### P1 ROS battery freshness regression (2026-11-08)
- rc_agent source commits [`bbf6202a`](https://github.com/ARDev1161/rc_agent/commit/bbf6202a3eb75f6bd5207296df48eecd5e50b54d), [`183908b1`](https://github.com/ARDev1161/rc_agent/commit/183908b121caefbcc40873fb910fd33577615024), [`a7c44e65`](https://github.com/ARDev1161/rc_agent/commit/a7c44e6546f1534c671f88dd4b15c55aef73ebb4): new actual ROS2 publisher/subscriber regression publishes 42%, waits for timeout-cleared outgoing Sensors, then publishes 76% to verify recovery. Registered as `battery_freshness_test` under colcon/ament GTest; no proto or application behavior change. AmurCore code unchanged in this cycle; peer plan updated for handoff. ROS2 Humble build/test and Qt5/loopback CI for these feature HEADs NOT VERIFIED; do not mark regression PASS until CI execution. P1 IN PROGRESS; P1-01..08 hardware/manual NOT RUN, P2 TODO. Next: inspect latest Actions runs/jobs and fix any test compilation/runtime errors, then add UI disconnect/offscreen coverage.


### P1 battery presentation + freshness regression follow-up (2026-11-08)
- AmurCore [3a80b038](https://github.com/ARDev1161/AmurCore/commit/3a80b038a1bcb36d3a10a6236acde3c009d8f8e2), [9d828a8e](https://github.com/ARDev1161/AmurCore/commit/9d828a8e5da4029e5db3bf688d4dfdf7720d3e79), [be7a30dc](https://github.com/ARDev1161/AmurCore/commit/be7a30dca44804562b13c74b4b370a41198e6bd3), [2654b486](https://github.com/ARDev1161/AmurCore/commit/2654b486d5941726d65598386492b85dce8b5bd0): extracted Qt battery display policy and added offscreen CTest for absent, 42%, charging, NaN, invalid percentages and absent again. No wire changes.
- rc_agent [a88bdc04](https://github.com/ARDev1161/rc_agent/commit/a88bdc042c18d731ec0905982a20378650ef197c) extends ROS2 battery freshness test with initial absence and post-recovery freshness checks.
- Source HEAD accepted by GitHub contents API. CI current HEAD NOT VERIFIED: GitHub connector exposes PR-triggered commit workflow runs only and combined status was empty; public Actions web fetch unavailable. Qt5/ROS2 local builds NOT RUN. Do not claim PASS. Next: inspect current feature-branch Actions job logs, fix any compile failures, and perform hardware P1-01..08 when robot available. P1 IN PROGRESS; P2 TODO. Documentation branches untouched, prior docs deploy success applies only to their documented SHAs.


### P1 battery concurrency and reconnect regression — 2026-11-08
- rc_agent [73f08412](https://github.com/ARDev1161/rc_agent/commit/73f0841247a6a0fb61173e799ac9f5430095503e) moves the 5-second freshness check and timestamp read under the same Sensors mutex as the ROS callback and gRPC snapshot, preventing a timestamp race under future multi-threaded executor configurations. No wire change.
- AmurCore [9a9b031b](https://github.com/ARDev1161/AmurCore/commit/9a9b031bc4343b9fd11863bc6ed13504f6ab770c) extends Qt offscreen battery presentation regression with absent→76% reconnect, 0% and 100% boundaries, then absent again; no production or proto changes.
- Source commits accepted by GitHub API. Local Qt/ROS2 toolchain tests NOT RUN; GitHub Actions HEAD results NOT VERIFIED: connector returns no commit statuses and public Actions pages are inaccessible. Do not infer green from prior SHA. P1 IN PROGRESS; manual P1-01..08 NOT RUN; P2 TODO. Next: inspect current Actions logs when accessible, fix any red jobs, then run simulator/live P1 gates. Docs branches unchanged.


### P1 battery regression extension — 2026-11-08
- rc_agent [`43d71c8e`](https://github.com/ARDev1161/rc_agent/commit/43d71c8e44a9f46121283d9b288f7d9a82ad2c7a) extends the real ROS2 publisher/subscriber/timer test to require a second expiration and third recovery (33%) after the previous 42%→timeout→76% cycle. This tests repeated timeout bookkeeping, not merely one-shot expiry.
- AmurCore [`d6e42fe6`](https://github.com/ARDev1161/AmurCore/commit/d6e42fe6e81c8af383be898cd58dcc97a359a65f) adds Qt offscreen presentation checks for positive and negative infinity, which must show unknown rather than fake charge.
- Local toolchains: colcon, ROS2 installation, qmake, and protoc unavailable; tests NOT RUN locally. GitHub commit-workflow query is PR-only and returns empty for feature HEAD, so Actions HEAD statuses NOT VERIFIED (not PASS). No protobuf/production changes, no docs branch changes. P1 IN PROGRESS, P1-01..08 hardware/live gates NOT RUN; P2 TODO. Next: obtain current feature-branch Actions job results and fix any failing build/test, then conduct simulator/live P1 acceptance.


### P1 battery refresh/status regression (2026-11-08)
- rc_agent [`e3f316f9`](https://github.com/ARDev1161/rc_agent/commit/e3f316f986452509a34202e384e37050939136f1) / [`3abd8285`](https://github.com/ARDev1161/rc_agent/commit/3abd8285ff34f504473eeaf79ed431a2e7dfc4a5) extend real ROS2 battery test with an in-window 55% refresh, ensuring it survives the previous sample's expiry tick. Test remains timing-sensitive on overloaded runners; investigate if flaky rather than weakening checks.
- AmurCore [`785c8c6f`](https://github.com/ARDev1161/AmurCore/commit/785c8c6fe68cb94e26a582de861346a5a0a7d25a) adds Qt offscreen FULL and UNKNOWN power-supply-status assertions: neither should incorrectly append charging to 100%.
- Shared proto/production code unchanged. No local Qt5/ROS2 toolchains; build and tests NOT RUN. Current feature HEAD Actions results NOT VERIFIED: available commit-workflow connector filters push runs. P1 IN PROGRESS, live P1-01..08 NOT RUN, P2 TODO. Next: inspect current Actions via browser/API when accessible, fix any failure; continue simulation gates, then hardware smoke tests.


### P1 cross-transport battery snapshot isolation (2026-11-08)
- AmurCore [`7afc6cbe`](https://github.com/ARDev1161/AmurCore/commit/7afc6cbeb8f65df64bcce5ad30759d9980fad6b0): expanded real gRPC unary loopback regression: a complete Sensors snapshot with no battery must clear the previous battery while retaining robot_info; a subsequent recovered 76% CHARGING sample must be accepted. No production/proto changes.
- rc_agent [`fb9f2b9a`](https://github.com/ARDev1161/rc_agent/commit/fb9f2b9afe35cfe311e88a0da830d6b73d7971d3): ROS2 BatteryState expiry/recovery regression now asserts repeated timeout clears only battery, not robot_info. No production/proto changes.
- Tests added, not executed locally (ROS2 Humble/Qt5/gRPC toolchains unavailable). GitHub commit workflow-run connector filters PR runs and cannot establish push CI status; current SHA CI NOT VERIFIED. P1 IN PROGRESS, hardware/live P1-01..08 NOT RUN; P2 TODO. Next: inspect feature-head Actions results and fix any failing jobs; continue deterministic transport/simulator P1 checks.
