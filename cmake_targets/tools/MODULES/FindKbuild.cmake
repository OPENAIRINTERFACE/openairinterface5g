# - Support for building kernel modules

# This module defines several variables which may be used when build
# kernel modules.
#
# The following variables are set here:
#  Kbuild_VERSION_STRING - kernel version.
#  Kbuild_ARCH - architecture.
#  Kbuild_BUILD_DIR - directory for build kernel and/or its components.
#  Kbuild_VERSION_{MAJOR|MINOR|TWEAK} - kernel version components
#  Kbuild_VERSION_STRING_CLASSIC - classic representation of version,
# where parts are delimited by dots.
#
# Cache variables which affect on this package:
#  CMAKE_SYSTEM_VERSION - change Kbuild_VERSION_STRING
# Also, setting CMAKE_SYSTEM_VERSION will be interpreted by cmake as crosscompile.
#
# Cache variables(ADVANCED) which affect on this package:
#  ARCH - if not empty, change Kbuild_ARCH
#  KBUILD_DIR - change Kbuild_BUILD_DIR

set(Kbuild_VERSION_STRING ${CMAKE_SYSTEM_VERSION})

# Form classic version view.
string(REGEX MATCH "([0-9]+)\\.([0-9]+)[.-]([0-9]+)"
    _kernel_version_match
    "${Kbuild_VERSION_STRING}"
)

if(NOT _kernel_version_match)
    message(FATAL_ERROR "Kernel version has unexpected format: ${Kbuild_VERSION_STRING}")
endif(NOT _kernel_version_match)

set(Kbuild_VERSION_MAJOR ${CMAKE_MATCH_1})
set(Kbuild_VERSION_MINOR ${CMAKE_MATCH_2})
set(Kbuild_VERSION_TWEAK ${CMAKE_MATCH_3})
# Version string for compare
set(Kbuild_VERSION_STRING_CLASSIC "${Kbuild_VERSION_MAJOR}.${Kbuild_VERSION_MINOR}.${Kbuild_VERSION_TWEAK}")

set(KBUILD_DIR "/lib/modules/${Kbuild_VERSION_STRING}/build" CACHE PATH
    "Directory for build linux kernel and/or its components."
)
mark_as_advanced(KBUILD_DIR)

set(Kbuild_BUILD_DIR "${KBUILD_DIR}")

set(ARCH "" CACHE STRING
    "Architecture for build linux kernel components for, empty string means autodetect."
)
mark_as_advanced(ARCH)

if(NOT ARCH)
    # Autodetect arch.
    execute_process(COMMAND uname -m
        RESULT_VARIABLE uname_m_result
        OUTPUT_VARIABLE ARCH_DEFAULT
    )
    if(NOT uname_m_result EQUAL 0)
        message("'uname -m' failed:")
        message("${ARCH_DEFAULT}")
        message(FATAL_ERROR "Failed to determine system architecture.")
    endif(NOT uname_m_result EQUAL 0)

    string(REGEX REPLACE "\n$" "" ARCH_DEFAULT "${ARCH_DEFAULT}")

    # Pairs of pattern-replace for postprocess architecture string from
    # 'uname -m'.
    # Taken from ./Makefile of the kernel build.
    set(_kbuild_arch_replacers
        "i.86" "x86"
        "x86_64" "x86"
        "sun4u" "sparc64"
        "arm.*" "arm"
        "sa110" "arm"
        "s390x" "s390"
        "parisc64" "parisc"
        "ppc.*" "powerpc"
        "mips.*" "mips"
        "sh[234].*" "sh"
        "aarch64.*" "arm64"
    )

    set(_current_pattern)
    foreach(p ${_kbuild_arch_replacers})
        if(_current_pattern)
            string(REGEX REPLACE "${_current_pattern}" "${p}" ARCH_DEFAULT "${ARCH_DEFAULT}")
            set(_current_pattern)
        else(_current_pattern)
            set(_current_pattern "${p}")
        endif(_current_pattern)
    endforeach(p ${_kbuild_arch_replacers})

    set(Kbuild_ARCH "${ARCH_DEFAULT}")
else(NOT ARCH)
    # User-provided value is used.
    set(Kbuild_ARCH "${ARCH}")
endif(NOT ARCH)



include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Kbuild
    REQUIRED_VARS Kbuild_BUILD_DIR
    VERSION_VAR KBuild_VERSION_STRING_CLASSIC
)
