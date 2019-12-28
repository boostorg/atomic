# Copyright 2019 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

include(BoostTest OPTIONAL RESULT_VARIABLE HAVE_BOOST_TEST)

if(NOT HAVE_BOOST_TEST)
  return()
endif()

if(WIN32)
  set(compile_definitions BOOST_USE_WINDOWS_H _WIN32_WINNT=0x0500)
endif()

boost_test(SOURCES native_api.cpp COMPILE_DEFINITIONS ${compile_definitions} LINK_LIBRARIES Boost::atomic Boost::core)
boost_test(SOURCES fallback_api.cpp COMPILE_DEFINITIONS ${compile_definitions} LINK_LIBRARIES Boost::atomic Boost::core)

# Boost::thread not available yet

# boost_test(SOURCES atomicity.cpp COMPILE_DEFINITIONS ${compile_definitions} LINK_LIBRARIES Boost::atomic Boost::thread)
# boost_test(SOURCES ordering.cpp COMPILE_DEFINITIONS ${compile_definitions} LINK_LIBRARIES Boost::atomic Boost::thread)

boost_test(SOURCES lockfree.cpp COMPILE_DEFINITIONS ${compile_definitions} LINK_LIBRARIES Boost::atomic Boost::core)

boost_test(TYPE compile-fail SOURCES cf_arith_void_ptr.cpp COMPILE_DEFINITIONS ${compile_definitions} LINK_LIBRARIES Boost::atomic)
boost_test(TYPE compile-fail SOURCES cf_arith_func_ptr.cpp COMPILE_DEFINITIONS ${compile_definitions} LINK_LIBRARIES Boost::atomic)
boost_test(TYPE compile-fail SOURCES cf_arith_mem_ptr.cpp COMPILE_DEFINITIONS ${compile_definitions} LINK_LIBRARIES Boost::atomic)

if(NOT CMAKE_VERSION VERSION_LESS 3.8) # for cxx_std_17

  boost_test(TYPE compile SOURCES c_implicit_ctor.cpp COMPILE_DEFINITIONS ${compile_definitions} COMPILE_FEATURES cxx_std_17 LINK_LIBRARIES Boost::atomic Boost::static_assert)

endif()
