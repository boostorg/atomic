# Copyright 2018 Mike Dev
# Copyright 2019 Peter Dimov
# Copyright 2020-2025 Andrey Semashev
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.5...3.16)
project(boost_atomic VERSION "${BOOST_SUPERPROJECT_VERSION}" LANGUAGES CXX)

include(CheckCXXSourceCompiles)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Note: We can't use the Boost::library targets in the configure checks as they may not yet be included
# by the superproject when this CMakeLists.txt is included. We also don't want to hardcode include paths
# of the needed libraries and their dependencies, recursively, as this is too fragile and requires maintenance.
# Instead, we collect include paths of all libraries and use them in the configure checks. This works faster
# if there is a unified Boost include tree in the filesystem (i.e. if `b2 headers` was run or we're in the
# official monolithic Boost distribution tree).
include(cmake/BoostLibraryIncludes.cmake)

set(boost_atomic_sources src/lock_pool.cpp)

set(CMAKE_REQUIRED_INCLUDES ${BOOST_LIBRARY_INCLUDES})
check_cxx_source_compiles("#include <${CMAKE_CURRENT_SOURCE_DIR}/../config/checks/architecture/x86.cpp>\nint main() {}" BOOST_ATOMIC_TARGET_X86)
unset(CMAKE_REQUIRED_INCLUDES)

if (BOOST_ATOMIC_TARGET_X86)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if (CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(boost_atomic_sse2_cflags "/arch:SSE2")
            set(boost_atomic_sse41_cflags "/arch:SSE2")
        endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        if (WIN32)
            set(boost_atomic_sse2_cflags "/QxSSE2")
            set(boost_atomic_sse41_cflags "/QxSSE4.1")
        else()
            set(boost_atomic_sse2_cflags "-xSSE2")
            set(boost_atomic_sse41_cflags "-xSSE4.1")
        endif()
    else()
        set(boost_atomic_sse2_cflags "-msse -msse2")
        set(boost_atomic_sse41_cflags "-msse -msse2 -msse3 -mssse3 -msse4.1")
    endif()

    set(CMAKE_REQUIRED_INCLUDES ${BOOST_LIBRARY_INCLUDES})
    set(CMAKE_REQUIRED_FLAGS "${boost_atomic_sse2_cflags}")
    check_cxx_source_compiles("#include <${CMAKE_CURRENT_SOURCE_DIR}/config/has_sse2.cpp>" BOOST_ATOMIC_COMPILER_HAS_SSE2)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_INCLUDES)

    set(CMAKE_REQUIRED_INCLUDES ${BOOST_LIBRARY_INCLUDES})
    set(CMAKE_REQUIRED_FLAGS "${boost_atomic_sse41_cflags}")
    check_cxx_source_compiles("#include <${CMAKE_CURRENT_SOURCE_DIR}/config/has_sse41.cpp>" BOOST_ATOMIC_COMPILER_HAS_SSE41)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_INCLUDES)

    if (BOOST_ATOMIC_COMPILER_HAS_SSE2)
        set(boost_atomic_sources_sse2 src/find_address_sse2.cpp)
        set_source_files_properties(${boost_atomic_sources_sse2} PROPERTIES COMPILE_FLAGS "${boost_atomic_sse2_cflags}")
        set(boost_atomic_sources ${boost_atomic_sources} ${boost_atomic_sources_sse2})
    endif()

    if (BOOST_ATOMIC_COMPILER_HAS_SSE41)
        set(boost_atomic_sources_sse41 src/find_address_sse41.cpp)
        set_source_files_properties(${boost_atomic_sources_sse41} PROPERTIES COMPILE_FLAGS "${boost_atomic_sse41_cflags}")
        set(boost_atomic_sources ${boost_atomic_sources} ${boost_atomic_sources_sse41})
    endif()
endif()

set(CMAKE_REQUIRED_INCLUDES ${BOOST_LIBRARY_INCLUDES})
check_cxx_source_compiles("#include <${CMAKE_CURRENT_SOURCE_DIR}/config/pthread_cond_clockwait.cpp>" BOOST_ATOMIC_HAS_PTHREAD_COND_CLOCKWAIT)
unset(CMAKE_REQUIRED_INCLUDES)

add_library(boost_atomic ${boost_atomic_sources})
add_library(Boost::atomic ALIAS boost_atomic)

target_compile_features(boost_atomic PUBLIC cxx_std_11)

target_include_directories(boost_atomic PUBLIC include)
target_include_directories(boost_atomic PRIVATE src)

target_link_libraries(boost_atomic
    PUBLIC
        Boost::assert
        Boost::config
        Boost::predef
        Boost::type_traits
    PRIVATE
        Boost::align
        Boost::preprocessor

        Threads::Threads
)

if (WIN32)
    target_link_libraries(boost_atomic
        PUBLIC
            Boost::winapi

            synchronization
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # MinGW-w64 defines _WIN32_WINNT to _WIN32_WINNT_WS03 by default, which disables WaitOnAddress API that is required by Boost.Atomic.
        # Define the macro ourselves to the Windows version required by Boost.Atomic.
        # https://github.com/boostorg/atomic/issues/73
        target_compile_definitions(boost_atomic PUBLIC _WIN32_WINNT=0x0A00)
    endif()
endif()

target_compile_definitions(boost_atomic
    PUBLIC
        BOOST_ATOMIC_NO_LIB
    PRIVATE
        BOOST_ATOMIC_SOURCE
)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(boost_atomic PUBLIC BOOST_ATOMIC_DYN_LINK)
else()
    target_compile_definitions(boost_atomic PUBLIC BOOST_ATOMIC_STATIC_LINK)
endif()

if (BOOST_ATOMIC_HAS_PTHREAD_COND_CLOCKWAIT)
    target_compile_definitions(boost_atomic PRIVATE BOOST_ATOMIC_HAS_PTHREAD_COND_CLOCKWAIT)
endif()

if (BOOST_ATOMIC_COMPILER_HAS_SSE2)
    target_compile_definitions(boost_atomic PRIVATE BOOST_ATOMIC_USE_SSE2)
endif()
if (BOOST_ATOMIC_COMPILER_HAS_SSE41)
    target_compile_definitions(boost_atomic PRIVATE BOOST_ATOMIC_USE_SSE41)
endif()

if (BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
    add_subdirectory(test)
endif()
