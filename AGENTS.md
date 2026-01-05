# Repository Guidelines

## Project Structure & Modules
- Core app: `main.cpp`, `amurcore.*`, UI layouts in `*.ui` (Qt Designer).
- Logic: `logic/` (nav dialog, map widget, movements, robot info).
- Network: `network/` (gRPC client/server, protobuf definitions in `network/protobuf`).
- Input: `joystick/` (state worker, adapter), threads helpers in `threads/`.
- Config & build: root `CMakeLists.txt`, `cmake/` helpers, `config/` for libconfig utilities, assets under `data/`.
- Build outputs: prefer `build/` (out-of-source). Keep `CMakeLists.txt.user`/`build/` untracked.

## Build, Test, Run
- Configure & build (Debug):
  ```bash
  cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
  cmake --build build
  ```
- Configure & build (Release): add `-DCMAKE_BUILD_TYPE=Release`.
- Run app (if GUI available): `./build/AmurCore`.
- Tests: none defined; add CTest targets in CMake if introducing tests.

## Coding Style & Naming
- Language: C++17/Qt. Follow existing patterns; 4-space indent, braces on new line for classes/functions as in current code.
- Naming: Classes `PascalCase`, methods `camelCase`, constants/macros `UPPER_SNAKE`. Prefer `std::` over raw macros; use RAII for locks.
- UI text uses Qt `tr()` for i18n. Keep Russian locale strings in `AmurCore_ru_RU.ts`.
- Avoid new macros for config; prefer enums/constexpr.

## Testing Guidelines
- Add unit/integration tests via CTest/GoogleTest if applicable; place under `tests/` or module-level `*_test.cpp`.
- Name tests after feature: `nav_follow_waypoints_test.cpp`; ensure deterministic, no network dependency when possible.
- Document how to run: `ctest --output-on-failure` from build dir.

## Commit & PR Guidelines
- Commit messages: use Conventional Commits (`feat:`, `fix:`, `refactor:`, `chore:`). Keep scope short (e.g., `nav`, `network`).
- Each commit should be focused (e.g., logic change vs. build tag bump).
- PRs (if used): include summary, testing done, affected modules, screenshots/gifs for UI changes, link issues. Avoid committing build artifacts or user-specific CMake cache files.

## Configuration & Security Tips
- Protobuf/gRPC versions are pinned in `cmake/grpc.cmake`; update deliberately.
- Keep network endpoints configurable via config files (see `AmurCore.cfg.in` / libconfig usage in `config/`).
- When handling mutexes around proto objects (`network/client.cpp`), use RAII locks to avoid races.
