# mtable

This demo is configured to build only with a **clang 26** toolchain.

## Configure

Use one of the presets from `CMakePresets.json`:

```bash
cmake --preset clang26-debug
cmake --preset clang26-relwithdebinfo
```

If your clang 26 installation is not on `PATH`, point the toolchain file at it:

```bash
cmake --preset clang26-debug -DMTABLE_CLANG26_ROOT=/path/to/clang26
```

or:

```bash
cmake --preset clang26-debug -DMTABLE_CLANG26_CXX=/path/to/clang++
```

## Build

```bash
cmake --build --preset clang26-debug
```

## Clean stale build trees

If an older build directory was configured with the wrong compiler, remove it or reconfigure it before running `cmake --build`:

```bash
rm -rf cmake-build-debug cmake-build-relwithdebinfo
cmake --preset clang26-debug
```

