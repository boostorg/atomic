#!/bin/bash

# Copyright 2020 Rene Rivera, Sam Darwin
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at http://boost.org/LICENSE_1_0.txt)

set -e
set -x
export TRAVIS_BUILD_DIR=$(pwd)
export DRONE_BUILD_DIR=$(pwd)
export TRAVIS_BRANCH=$DRONE_BRANCH
export VCS_COMMIT_ID=$DRONE_COMMIT
export GIT_COMMIT=$DRONE_COMMIT
export REPO_NAME=$DRONE_REPO
export PATH=~/.local/bin:/usr/local/bin:$PATH

if [ "$DRONE_JOB_BUILDTYPE" == "boost" ]; then

echo '==================================> INSTALL'

GIT_FETCH_JOBS=8
BOOST_BRANCH=develop
if [ "$TRAVIS_BRANCH" = "master" ]; then BOOST_BRANCH=master; fi
cd ..
git clone -b $BOOST_BRANCH --depth 1 https://github.com/boostorg/boost.git boost-root
cd boost-root
git submodule init tools/boostdep
git submodule init tools/build
git submodule init tools/boost_install
git submodule init libs/headers
git submodule init libs/config
git submodule update --jobs $GIT_FETCH_JOBS
cp -r $TRAVIS_BUILD_DIR/* libs/atomic
python tools/boostdep/depinst/depinst.py --git_args "--jobs $GIT_FETCH_JOBS" atomic
if [ -z "$TEST_CMAKE" ]; then ./bootstrap.sh; ./b2 headers; fi

echo '==================================> SCRIPT'

echo "using $TOOLSET : : $COMPILER ;" > ~/user-config.jam
BUILD_JOBS=`(nproc || sysctl -n hw.ncpu) 2> /dev/null`
if [ -z "$BUILD_VARIANT" ]; then BUILD_VARIANT="debug,release"; fi
if [ -n "$CXXSTD64" ]; then echo ""; echo "Testing 64-bit targets"; echo ""; ./b2 -j $BUILD_JOBS libs/atomic/test toolset=$TOOLSET variant=$BUILD_VARIANT address-model=64 cxxstd=$CXXSTD64 ${UBSAN:+cxxflags=-fsanitize=undefined cxxflags=-fno-sanitize-recover=undefined linkflags=-fsanitize=undefined define=UBSAN=1 debug-symbols=on} ${INSTRUCTION_SET:+instruction-set="$INSTRUCTION_SET"} ${CXXFLAGS:+cxxflags="$CXXFLAGS"} ${LINKFLAGS:+linkflags="$LINKFLAGS"}; fi
if [ -n "$CXXSTD32" ]; then echo ""; echo "Testing 32-bit targets"; echo ""; ./b2 -j $BUILD_JOBS libs/atomic/test toolset=$TOOLSET variant=$BUILD_VARIANT address-model=32 cxxstd=$CXXSTD32 ${UBSAN:+cxxflags=-fsanitize=undefined cxxflags=-fno-sanitize-recover=undefined linkflags=-fsanitize=undefined define=UBSAN=1 debug-symbols=on} ${INSTRUCTION_SET:+instruction-set="$INSTRUCTION_SET"} ${CXXFLAGS:+cxxflags="$CXXFLAGS"} ${LINKFLAGS:+linkflags="$LINKFLAGS"}; fi

elif [ "$DRONE_JOB_BUILDTYPE" == "d9bb5aab6f-ee47089688" ]; then

echo '==================================> INSTALL'

GIT_FETCH_JOBS=8
BOOST_BRANCH=develop
if [ "$TRAVIS_BRANCH" = "master" ]; then BOOST_BRANCH=master; fi
cd ..
git clone -b $BOOST_BRANCH --depth 1 https://github.com/boostorg/boost.git boost-root
cd boost-root
git submodule init tools/boostdep
git submodule init tools/build
git submodule init tools/boost_install
git submodule init libs/headers
git submodule init libs/config
git submodule update --jobs $GIT_FETCH_JOBS
cp -r $TRAVIS_BUILD_DIR/* libs/atomic
python tools/boostdep/depinst/depinst.py --git_args "--jobs $GIT_FETCH_JOBS" atomic
if [ -z "$TEST_CMAKE" ]; then ./bootstrap.sh; ./b2 headers; fi

echo '==================================> SCRIPT'

BUILD_JOBS=`(nproc || sysctl -n hw.ncpu) 2> /dev/null`
mkdir __build_static__ && cd __build_static__
cmake ../libs/atomic/test/test_cmake
cmake --build . --target boost_atomic_cmake_self_test -j $BUILD_JOBS
cd ..
mkdir __build_shared__ && cd __build_shared__
cmake -DBUILD_SHARED_LIBS=On ../libs/atomic/test/test_cmake
cmake --build . --target boost_atomic_cmake_self_test -j $BUILD_JOBS

fi
