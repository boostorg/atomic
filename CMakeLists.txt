# Copyright 2018 Mike Dev
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.5)
project(BoostAtomic LANGUAGES CXX)

add_library(boost_atomic src/lockpool.cpp)
add_library(Boost::atomic ALIAS boost_atomic)

target_include_directories(boost_atomic PUBLIC include)

target_link_libraries(boost_atomic
    PUBLIC
        Boost::assert
        Boost::config
        Boost::type_traits
)

