# QNX x86_64 toolchain file
# Requires QNX_HOST and QNX_TARGET environment variables to be set.
# Typical QNX SDP 7.x / 8.x layout:
#   QNX_HOST   = <sdp>/host/linux/x86_64   (or win64/x86_64 on Windows)
#   QNX_TARGET = <sdp>/target/qnx7          (or qnx8)

set(CMAKE_SYSTEM_NAME      QNX)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Resolve QNX_HOST from the environment
if(DEFINED ENV{QNX_HOST})
    file(TO_CMAKE_PATH "$ENV{QNX_HOST}" QNX_HOST)
else()
    message(FATAL_ERROR "QNX_HOST environment variable is not set. "
                        "Source the QNX SDP environment script first.")
endif()

if(DEFINED ENV{QNX_TARGET})
    file(TO_CMAKE_PATH "$ENV{QNX_TARGET}" QNX_TARGET)
else()
    message(FATAL_ERROR "QNX_TARGET environment variable is not set. "
                        "Source the QNX SDP environment script first.")
endif()

# Executable suffix (.exe on Windows host, empty on Linux/macOS)
if(CMAKE_HOST_WIN32)
    set(_EXE ".exe")
else()
    set(_EXE "")
endif()

# CPU variant: x86_64 (nto x86_64)
set(QNX_CPU_VARIANT  "x86_64")
set(QNX_NTO_VARIANT  "gcc_nto${QNX_CPU_VARIANT}")

# Compiler wrappers
set(CMAKE_C_COMPILER   "${QNX_HOST}/usr/bin/qcc${_EXE}"            CACHE PATH "QNX C compiler")
set(CMAKE_CXX_COMPILER "${QNX_HOST}/usr/bin/q++${_EXE}"            CACHE PATH "QNX C++ compiler")
set(CMAKE_AR           "${QNX_HOST}/usr/bin/ntox86_64-ar${_EXE}"   CACHE PATH "")
set(CMAKE_RANLIB       "${QNX_HOST}/usr/bin/ntox86_64-ranlib${_EXE}" CACHE PATH "")
set(CMAKE_STRIP        "${QNX_HOST}/usr/bin/ntox86_64-strip${_EXE}" CACHE PATH "")

# Drive qcc/q++ to the correct CPU variant
set(CMAKE_C_COMPILER_TARGET   "gcc_nto${QNX_CPU_VARIANT}")
set(CMAKE_CXX_COMPILER_TARGET "gcc_nto${QNX_CPU_VARIANT}")

# Let qcc know the variant via -V flag
string(APPEND CMAKE_C_FLAGS_INIT   " -V${QNX_NTO_VARIANT}")
string(APPEND CMAKE_CXX_FLAGS_INIT " -V${QNX_NTO_VARIANT}")

# Sysroot
set(CMAKE_SYSROOT "${QNX_TARGET}")

set(CMAKE_FIND_ROOT_PATH "${QNX_TARGET}/x86_64")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
