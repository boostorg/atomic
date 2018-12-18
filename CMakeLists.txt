# Copyright 2018 Peter Dimov
# Copyright 2018 Andrey Semashev
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

# Partial (add_subdirectory only) and experimental CMake support
# Subject to change; please do not rely on the contents of this file yet.
# This file is NOT supposed to be used to build the library, use Boost.Build for that.

cmake_minimum_required(VERSION 3.5)

project(BoostAtomic LANGUAGES CXX)

add_library(boost_atomic INTERFACE)
add_library(Boost::atomic ALIAS boost_atomic)

target_include_directories(boost_atomic INTERFACE include)

target_link_libraries(boost_atomic
    INTERFACE
        Boost::assert
        Boost::config
        Boost::type_traits
)
