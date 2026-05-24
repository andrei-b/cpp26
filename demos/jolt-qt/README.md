# Qt 6 + Jolt Physics + reflected signal/slot demo

This demo uses Jolt Physics as an external non-Qt engine and updates a Qt `QGraphicsScene` through your custom `Signal<T...>::connect_reflected_direct(obj, "methodName")` path.

## What it demonstrates

- Jolt owns the physics bodies.
- `JoltWorld` emits plain C++ signals after each physics step.
- `SceneBody` is a small C++ wrapper around ordinary `QGraphicsItem` primitives.
- Connections are made by **method name strings**, for example:

```cpp
world.bodyPositionChanged.connect_reflected_direct(ballView, "setCenter");
world.bodyAngleChanged.connect_reflected_direct(ballView, "setAngleDegrees");
world.bodySpeedChanged.connect_reflected_direct(ballView, "setSpeedText");
```

No Qt `QObject` slots are required on the graphics body.

## Dependencies

- Qt 6 Widgets
- Jolt Physics (either installed with CMake package export, or fetched automatically from GitHub)
- Reflection-enabled Clang/C++26 toolchain matching your existing `method_table.hpp`

Example build:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/path/to/qt" \
  -DCLANG21_ROOT=/opt/clang-p2996
cmake --build build -j
./build/jolt_qgraphics_reflected_slots
```

If you already have Jolt installed, you can still point CMake to it with `-DJolt_DIR=/path/to/JoltConfig.cmake/dir` or by adding its install prefix to `CMAKE_PREFIX_PATH`.
