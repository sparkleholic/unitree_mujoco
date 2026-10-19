# Go2 StandUp/StandDown Service Deployments — Implementation Plan

Created: 2026-10-19
Status: Awaiting user review / not implemented
Requirements: [go2-sport-service-modes](../req/2026-10-19-go2-sport-service-modes.md)

## Goal and Execution

Support existing LowCmd control, an external adapter, and an internal server, and verify real posture changes in the same Go2 scene. Share one SDK service wrapper and posture controller across both service deployments. The primary agent implements sequentially; do not delegate or commit without a user request.

## Global Constraints

- Use C++17/CMake and the current SDK2 archive. Do not modify the SDK or external reconstructed sources.
- Build the simulator in `simulate/build/`. Preserve user CMake changes and `_test/`.
- Default to lowcmd and preserve existing invocation and YAML compatibility.
- Limit the service to Go2 GO-IDL 12-joint models and StandUp/StandDown actions.
- Use a 2.0-second smoothstep, initial `kp=50`, `kd=3.5`, `dq=0`, `tau=0`; a 2ms external period; and a 500ms stale-state threshold.
- Run one server/LowCmd writer per domain. Isolate tests using loopback and a separate domain.
- Trace requirements R1–R12 and verification V1–V7. Documentation creation alone does not constitute implementation completion.
- Write all project documents in English. Consolidate working documents into completed and delete the feature's req/plan/wip only after implementation and verification are complete.

## Planned Files and Responsibilities

These paths are proposed. If implementation reveals a better arrangement, record the reason in wip and update this table.

| File | Responsibility |
| --- | --- |
| `common/go2_posture_controller.h` | SDK/MuJoCo-independent posture targets, interpolation, transition, and hold state. |
| `common/go2_sport_server.h` | SDK Server subclass, two API registrations, command-acceptance callback, and error responses. |
| `common/go2_lowcmd.h` | GO LowCmd initialization, 12-joint command assignment, and CRC. |
| `example/cpp/stand_go2.cpp` | Preserve existing behavior; select API/action; invoke actual SportClient; finite execution. |
| `example/cpp/go2_sport_adapter.cpp` | Feedback subscription, readiness/timeout, standalone service, and LowCmd loop. |
| `example/cpp/CMakeLists.txt` | Adapter, unit tests, state probe, and C++17 configuration. |
| `simulate/src/unitree_sdk2_bridge.h` | Internal service lifetime, control-mode selection, and shared controller integration. |
| `simulate/src/param.h`, `simulate/config.yaml` | Control default, CLI, and validation. |
| `simulate/src/main.cc` | Physics synchronization, headless execution if needed, and shutdown lifetimes. |
| `simulate/CMakeLists.txt` | Shared includes and required build integration. |
| `tests/go2_posture_test.cpp`, `tests/go2_service_test.cpp` | Interpolation, reversal, unsupported APIs, and readiness tests. |
| `tests/go2_state_probe.cpp` | Actual DDS feedback sampling and RMSE/height/timing evaluation. |
| `tests/run_go2_sport_modes.py` | Baseline and three-mode orchestration, process cleanup, JSON/log evidence. |
| `docs/completed/01_go2-sport-service-modes.md` | Final design, commands, and verified results after actual completion. |
| `docs/design.md`, `readme.md` | Current architecture and usage instructions. |

## Key Interfaces

Use `Posture = std::array<double, 12>` and `enum class StandPose { Up, Down }`.

- `Go2PostureController::Begin(StandPose pose, const Posture& measured, double time_seconds)` starts a command.
- `Go2PostureController::Sample(double time_seconds) const -> Posture` returns the current target.
- `Go2PostureController::Reset()` invalidates an active command; `Active() const -> bool` indicates whether to publish/apply control.
- `Go2SportServer(std::function<int32_t(StandPose)> accept)` dispatches requests to the acceptance callback. Register the version and two APIs in `Init()`, then receive requests through SDK `Start()`.
- Service callbacks never access MuJoCo data. They update mutex-protected pending commands/readiness only. The control loop owns measured-posture capture and `Begin()`.
- The external feedback callback updates mutex-protected posture/receive-time state; the control loop computes from a consistent snapshot.

## Review Focus

1. False success from RPC `0` without motion: evaluate actual LowState and body height under V3.
2. Publishing from missing/stale feedback: reject before readiness, stop on timeout, and require a new command after reconnect.
3. Conflicting writers: ignore external LowCmd in internal sport mode and use a single writer externally.
4. Invalid object/physics access during initialization, reset, or shutdown: test synchronization, reset, and callback lifetimes.
5. Clock differences and command reversal: use simulation time internally, monotonic time externally, and restart from measured posture.

## Task 1 — Runtime Baseline and Failure Evidence

Requirements: R2, R5–R6, V2, V7.

- [ ] Write the state probe and runner to collect actual DDS feedback and calculate target RMSE/height. Missing feedback, NaN, and timeout must fail.
- [ ] Use a test linked to the actual SDK SportClient to collect StandUp/StandDown return codes with explicit domain/network and bounded timeout.
- [ ] Run the unmodified simulator in an isolated domain and record the expected service-call failure baseline. Do not treat absence of a server as successful service execution.
- [ ] Inspect existing user processes and verify that runner cleanup terminates only PIDs it created.

## Task 2 — Shared Posture Controller and Service Contract

Requirements: R5–R8, R12, V1, V4–V5.

- [ ] First write and observe failing tests: initial sample equals measured posture, 1 second yields the midpoint, at/after 2 seconds yields the target, reversal restarts from new measured posture, and reset deactivates control.
- [ ] Implement the existing example's 12-joint targets and smoothstep `3*u*u - 2*u*u*u`, where `u=clamp((t-t0)/2, 0, 1)`. Treat time regression as a reset condition.
- [ ] First write and observe failing actual SDK client/server tests for both action callbacks, unready result `3202`, and unsupported Move result `3203`.
- [ ] Implement the shared service wrapper and LowCmd/CRC helper and pass the tests. Handlers and callback state must be valid before server startup.

## Task 3 — External Adapter and Client Modes

Requirements: R1–R2, R4, R6–R9, R11–R12, V1, V3–V5.

- [ ] Write tests for command rejection without feedback, acceptance with valid feedback, publication stopping at 500ms stale, no automatic resumption after reconnect, and command reversal.
- [ ] Implement adapter feedback subscription, control loop, and service. Use actual measured state in the 2ms loop; stop callbacks/loops and destroy owned objects in a valid order.
- [ ] Add `--api`, `--action`, `--domain`, `--network`, `--auto`, and `--duration` to stand_go2. Preserve its positional network argument and default interactive lowcmd behavior. Invalid arguments must fail.
- [ ] The sequence action leaves sufficient observation time after StandUp before StandDown. Sport mode creates no LowCmd publisher and checks RPC return codes.
- [ ] Run the original lowcmd simulator, adapter, and SportClient to verify V3. Retain final 0.5-second RMSE and height samples in result JSON.

## Task 4 — In-Process Simulator Server

Requirements: R3, R5–R8, R10, V1, V3–V6.

- [ ] Write configuration tests first: default lowcmd, omitted YAML key, invalid control, and rejection of HG/non-Go2/non-12-joint sport configurations.
- [ ] Connect the shared service/controller in internal sport mode. Preserve lowcmd mode and exclude external LowCmd from sport-mode control inputs.
- [ ] Examine bridge and physics/render state sharing and protect it with the same simulation mutex. Callbacks update pending commands only. Stop/recreate objects safely on model replacement and reset the controller on simulation-time reset.
- [ ] Check GUI availability. Use the existing GUI simulator if possible; otherwise add `--headless` to run the same bridge, model, and physics loop. Do not substitute a mock physics server.
- [ ] Run simulator sport mode plus SportClient and evaluate V3. Publish conflicting external LowCmd to confirm it cannot override internal control. Verify pause/reset and shutdown.
- [ ] Check existing GO/HG lowcmd build and initialization behavior.

## Task 5 — Full Verification and Documentation Completion

Requirements: V1–V7 and documentation deliverables.

- [ ] Run `cmake -S simulate -B simulate/build` and `cmake --build simulate/build -j2`.
- [ ] Run `cmake -S example/cpp -B example/cpp/build`, `cmake --build example/cpp/build -j2`, and `ctest --test-dir example/cpp/build --output-on-failure`.
- [ ] Run `python3 tests/run_go2_sport_modes.py --simulator simulate/build/unitree_mujoco --bin-dir example/cpp/build --network lo --domain 71 --output tests/build/go2-sport-results` to verify the baseline, direct LowCmd, external adapter, and internal server sequentially. Treat the output directory as generated artifacts.
- [ ] Check V3: at most 8 seconds per posture, RMSE ≤ 0.15rad, height difference ≥ 0.08m, plus failure-path results. Investigate and fix failures, then rerun affected checks.
- [ ] After all acceptance criteria are met, consolidate req/plan/wip into the English completed record with requirements, final Mermaid design, actual commands, measurements/logs, lifetimes/ownership, deviations, limitations, completion date, and uncommitted status.
- [ ] Update design/readme to the actual implementation and replace transient-document references with the completed link.
- [ ] Delete every req, plan, and wip document for this slug, including date variants. Do not retain completed-status stubs or remove other active work.
- [ ] Verify that the completed record stands alone, no links target deleted documents, no transient files for this slug remain, and the code and permanent documentation agree. Run `git diff --check`.

## Proposed Invocation Examples

These commands describe planned interfaces, not existing functionality. Confirm final commands with actual execution in completed. Stop each configuration before starting another.

```sh
# A. Direct control: two terminals
simulate/build/unitree_mujoco --control lowcmd -r go2 -i 71 -n lo
example/cpp/build/stand_go2 --api lowcmd --action sequence --auto --duration 12 --domain 71 --network lo

# B. External adapter: three terminals
simulate/build/unitree_mujoco --control lowcmd -r go2 -i 71 -n lo
example/cpp/build/go2_sport_adapter --domain 71 --network lo
example/cpp/build/stand_go2 --api sport --action sequence --auto --duration 12 --domain 71 --network lo

# C. Internal server: two terminals
simulate/build/unitree_mujoco --control sport -r go2 -i 71 -n lo
example/cpp/build/stand_go2 --api sport --action sequence --auto --duration 12 --domain 71 --network lo
```
