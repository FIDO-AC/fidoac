cmake_minimum_required(VERSION 2.8)

# Change the compiler BEFORE the first `project()` command to avoid an infinite loop.
# See: https://public.kitware.com/pipermail/cmake/2009-November/033133.html
if(APPLE)
  # If custom llvm compilers are available, use them (for openmp
  # support), otherwise disable multicore.
  if(EXISTS "/usr/local/opt/llvm/bin/clang")
    set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
  endif()
endif()

project (libsnark)

# Versionning of the project
set(LIBSNARK_VERSION_MAJOR 0)
set(LIBSNARK_VERSION_MINOR 1)

set(
  CURVE
  "BN128"
  CACHE
  STRING
  "Default curve: one of ALT_BN128, BN128, EDWARDS, MNT4, MNT6"
)

option(
  DEBUG
  "Enable debugging mode"
  OFF
)
option(
  LOWMEM
  "Limit the size of multi-exponentiation tables, for low-memory platforms"
  OFF
)
option(
  MULTICORE
  "Enable parallelized execution, using OpenMP"
  OFF
)
option(
  BINARY_OUTPUT
  "In serialization, output raw binary data (instead of decimal), which is smaller and faster."
  ON
)
option(
  MONTGOMERY_OUTPUT
  "Serialize Fp elements as their Montgomery representations (faster but not human-readable)"
  ON
)
option(
  USE_PT_COMPRESSION
  "Use point compression"
  ON
)
option(
  PROFILE_OP_COUNTS
  "Collect counts for field and curve operations"
  OFF
)

option(
  WITH_SUPERCOP
  "Support for Ed25519 signatures required by ADSNARK"
  ON
)

option(
  WITH_PROCPS
  "Use procps for memory profiling"
  ON
)

option(
  CPPDEBUG
  "Enable debugging of C++ STL (does not imply DEBUG)"
  OFF
)

option(
  PERFORMANCE
  "Enable link-time and aggressive optimizations"
  OFF
)

option(
  USE_ASM
  "Use architecture-specific optimized assembly code"
  ON
)

option(
  IS_LIBSNARK_PARENT
  "Libsnark parent folder option"
  ON
)

set(
  OPT_FLAGS
  ""
  CACHE
  STRING
  "Override C++ compiler optimization flags"
)

if("${DEPENDS_DIR}" STREQUAL "")
  set(
    DEPENDS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/depends"
    CACHE
    STRING
    "Optionally specify the dependency installation directory relative to the source directory (default: inside dependency folder)"
  )
else()
  set(DEPENDS_DIR "${DEPENDS_DIR}")
endif()

if(APPLE)
  set(WITH_PROCPS OFF)
  set(WITH_SUPERCOP OFF)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "^(Apple)?Clang$")
  # Common compilation flags and warning configuration
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wfatal-errors -Wno-delete-non-virtual-dtor"
  )
  if("${MULTICORE}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
  endif()
  # Default optimizations flags (to override, use -DOPT_FLAGS=...)
  if("${OPT_FLAGS}" STREQUAL "")
    if (NOT ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"))
      set(
        OPT_FLAGS
        "-ggdb3 -O2 -march=native -mtune=native"
        )
    endif()
  endif()
endif()

set(
  CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} ${OPT_FLAGS}"
)

# GMP
find_path(GMP_INCLUDE_DIR NAMES gmp.h)
find_library(GMP_LIBRARIES NAMES gmp libgmp)
find_library(GMPXX_LIBRARIES NAMES gmpxx libgmpxx)

include(FindPkgConfig)
pkg_check_modules(
  CRYPTO
  REQUIRED

  libcrypto
)

if("${WITH_PROCPS}")
  pkg_check_modules(
    PROCPS
    REQUIRED

    libprocps
  )
else()
  add_definitions(
    -DNO_PROCPS
  )
endif()

# Enable Boost for program_options
FIND_PACKAGE( Boost 1.40 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

add_definitions(
  -DCURVE_${CURVE}
)

enable_testing()

include_directories(.)

if(${CURVE} STREQUAL "BN128")
  add_definitions(
    -DBN_SUPPORT_SNARK=1
  )
endif()

if("${DEBUG}")
  add_definitions(-DDEBUG=1)
endif()

if("${LOWMEM}")
  add_definitions(-DLOWMEM=1)
endif()

if("${MULTICORE}")
  add_definitions(-DMULTICORE=1)
endif()

if("${BINARY_OUTPUT}")
  add_definitions(-DBINARY_OUTPUT)
endif()

if("${MONTGOMERY_OUTPUT}")
  add_definitions(-DMONTGOMERY_OUTPUT)
endif()

if(NOT "${USE_PT_COMPRESSION}")
  add_definitions(-DNO_PT_COMPRESSION=1)
endif()

if("${PROFILE_OP_COUNTS}")
  add_definitions(-DPROFILE_OP_COUNTS=1)
endif()

if("${CPPDEBUG}")
  add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
endif()

if("${PERFORMANCE}")
  add_definitions(-DNDEBUG)
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -flto -fuse-linker-plugin"
  )
  set(
    CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -flto"
  )
endif()

if("${USE_ASM}")
  add_definitions(-DUSE_ASM)
endif()

if("${USE_LINKED_LIBRARIES}")
  # libfqfft
  find_path(LIBFQFFT_INCLUDE_DIR NAMES libfqfft)
  set(LIBFQFFT_DIRECTORY ${LIBFQFFT_INCLUDE_DIR}/libfqfft)
  include_directories(${LIBFQFFT_DIRECTORY})

  # libff
  find_path(LIBFF_INCLUDE_DIR NAMES libff)
  include_directories(${LIBFF_INCLUDE_DIR}/libff)
  find_library(LIBFF_LIBRARIES NAMES ff libff)
endif()

# Configure CCache if available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if ("${IS_LIBSNARK_PARENT}")
  # If the option -DGEN_DOC is used, add the `docs` target
  if("${GEN_DOC}")
    include(cmake/documentation.cmake)
  endif()

  # Add a `make check` target that builds and tests
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
endif()

add_subdirectory(depends)
add_subdirectory(libsnark)
