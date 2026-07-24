# SelfReflected — C++26 Reflection Example

A small example demonstrating static reflection in C++26.

Any class derived from `SelfReflected` can automatically serialize its
properties to JSON. To expose a property, simply provide a zero-argument getter
whose name starts with `get_`.

## Example

```cpp
class Sensor : public SelfReflected<Sensor>
{
public:
    int get_temperature() const
    {
        return 23;
    }

    std::string get_name() const
    {
        return "Outdoor sensor";
    }
};
```

The `get_` prefix is removed when generating the JSON property name:

```cpp
Sensor sensor;
std::cout << sensor.to_json().dump(4);
```

Output:

```json
{
    "name": "Outdoor sensor",
    "temperature": 23
}
```

## Getter requirements

A reflected getter should:

- start with `get_`;
- take no arguments;
- be callable on a `const` object;
- return a value supported by the JSON library;
- be accessible from `SelfReflected`, normally by being declared `public`.

Methods that do not satisfy these conditions are ignored.

## Requirements

- A compiler with support for the proposed C++26 reflection features
- CMake
- `nlohmann/json`

The example can be tested with a reflection-enabled Clang 21 build, such as the
experimental P2996 implementation, or with a compatible compiler on Compiler
Explorer.

> [!NOTE]
> A regular Clang 21 installation may not include the experimental reflection
> implementation.

## Building

Set `CLANG21_ROOT` to the root directory of the reflection-enabled compiler:

```bash
cmake -S . -B build \
    -DCLANG21_ROOT=/opt/clang-p2996

cmake --build build
```

For a custom installation:

```bash
cmake -S . -B build \
    -DCLANG21_ROOT=/path/to/clang-p2996
```

Depending on the compiler layout, you may also need to adjust
`CLANG21_LIBDIR` and the compiler include directories in `CMakeLists.txt`.

## Status

C++26 reflection support is still experimental in currently available
compilers. Compiler syntax, build options, and library paths may change as
implementations evolve.