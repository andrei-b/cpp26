set(MTABLE_CLANG26_ROOT "" CACHE PATH "Root directory of the clang 26 toolchain")
set(MTABLE_CLANG26_CXX "" CACHE FILEPATH "Path to clang++ 26")

if(MTABLE_CLANG26_CXX)
    get_filename_component(_mtable_clang_bin_dir "${MTABLE_CLANG26_CXX}" DIRECTORY)
    get_filename_component(_mtable_clang_root "${_mtable_clang_bin_dir}" DIRECTORY)
elseif(MTABLE_CLANG26_ROOT)
    set(_mtable_clang_bin_dir "${MTABLE_CLANG26_ROOT}/bin")
    set(_mtable_clang_root "${MTABLE_CLANG26_ROOT}")
    set(MTABLE_CLANG26_CXX "${_mtable_clang_bin_dir}/clang++")
else()
    find_program(MTABLE_CLANG26_CXX
            NAMES clang++-26 clang++
            PATHS ENV PATH
            NO_DEFAULT_PATH)

    if(MTABLE_CLANG26_CXX)
        get_filename_component(_mtable_clang_bin_dir "${MTABLE_CLANG26_CXX}" DIRECTORY)
        get_filename_component(_mtable_clang_root "${_mtable_clang_bin_dir}" DIRECTORY)
    endif()
endif()

if(NOT MTABLE_CLANG26_CXX OR NOT EXISTS "${MTABLE_CLANG26_CXX}")
    message(FATAL_ERROR
            "clang++ 26 was not found. Set MTABLE_CLANG26_ROOT or MTABLE_CLANG26_CXX to a clang 26 installation.")
endif()

set(_mtable_clang_c "${_mtable_clang_bin_dir}/clang")
if(NOT EXISTS "${_mtable_clang_c}")
    message(FATAL_ERROR
            "clang was not found next to clang++ at ${_mtable_clang_c}")
endif()

set(CMAKE_C_COMPILER "${_mtable_clang_c}" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${MTABLE_CLANG26_CXX}" CACHE FILEPATH "" FORCE)

foreach(_mtable_llvm_tool IN ITEMS ar ranlib nm addr2line objcopy objdump strip dlltool)
    find_program(_mtable_llvm_path
            NAMES "llvm-${_mtable_llvm_tool}" "${_mtable_llvm_tool}"
            PATHS "${_mtable_clang_bin_dir}"
            NO_DEFAULT_PATH)
    if(_mtable_llvm_path)
        if(_mtable_llvm_tool STREQUAL "ar")
            set(CMAKE_AR "${_mtable_llvm_path}" CACHE FILEPATH "" FORCE)
        elseif(_mtable_llvm_tool STREQUAL "ranlib")
            set(CMAKE_RANLIB "${_mtable_llvm_path}" CACHE FILEPATH "" FORCE)
        elseif(_mtable_llvm_tool STREQUAL "nm")
            set(CMAKE_NM "${_mtable_llvm_path}" CACHE FILEPATH "" FORCE)
        elseif(_mtable_llvm_tool STREQUAL "addr2line")
            set(CMAKE_ADDR2LINE "${_mtable_llvm_path}" CACHE FILEPATH "" FORCE)
        elseif(_mtable_llvm_tool STREQUAL "objcopy")
            set(CMAKE_OBJCOPY "${_mtable_llvm_path}" CACHE FILEPATH "" FORCE)
        elseif(_mtable_llvm_tool STREQUAL "objdump")
            set(CMAKE_OBJDUMP "${_mtable_llvm_path}" CACHE FILEPATH "" FORCE)
        elseif(_mtable_llvm_tool STREQUAL "strip")
            set(CMAKE_STRIP "${_mtable_llvm_path}" CACHE FILEPATH "" FORCE)
        elseif(_mtable_llvm_tool STREQUAL "dlltool")
            set(CMAKE_DLLTOOL "${_mtable_llvm_path}" CACHE FILEPATH "" FORCE)
        endif()
    endif()
endforeach()

find_program(CMAKE_LINKER
        NAMES ld.lld lld
        PATHS "${_mtable_clang_bin_dir}"
        NO_DEFAULT_PATH)

