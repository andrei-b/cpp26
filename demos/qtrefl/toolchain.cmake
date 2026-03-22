set(CMAKE_C_COMPILER "/home/andrei/Desktop/clang26/build/bin/clang")
set(CMAKE_CXX_COMPILER "/home/andrei/Desktop/clang26/build/bin/clang++")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Let CMake search standard system prefixes too
set(CMAKE_PREFIX_PATH "/usr;/usr/lib/x86_64-linux-gnu/cmake")

# Compile flags
set(CMAKE_CXX_FLAGS_INIT
    "-stdlib=libc++ \
     -freflection \
     -fexpansion-statements \
     -Wall -Wextra -Wpedantic \
     -I/home/andrei/Desktop/clang26/build/include/c++/v1 \
     -I/home/andrei/Desktop/clang26/build/include/x86_64-unknown-linux-gnu/c++/v1"
)

# Link flags for executables
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-stdlib=libc++ \
     -L/home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu \
     -Wl,-rpath,/home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu \
     -lc++ -lc++abi -lunwind"
)

# Link flags for shared libs
set(CMAKE_SHARED_LINKER_FLAGS_INIT
    "-stdlib=libc++ \
     -L/home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu \
     -Wl,-rpath,/home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu \
     -lc++ -lc++abi -lunwind"
)
