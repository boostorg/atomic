//  Copyright (c) 2020-2025 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ATOMIC_TEST_TEST_CLOCK_HPP_INCLUDED_
#define BOOST_ATOMIC_TEST_TEST_CLOCK_HPP_INCLUDED_

#include <boost/config.hpp>
#if defined(BOOST_WINDOWS)
#include <boost/winapi/config.hpp>
#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/time.hpp>
#include <ratio>
#endif
#include <chrono>

#if defined(BOOST_WINDOWS)

// On Windows high precision clocks tend to cause spurious test failures because threads wake up earlier than expected.
// Use a lower precision steady clock for tests.
struct test_clock
{
    using rep = long long;
    using period = std::milli;
    using duration = std::chrono::duration< rep, period >;
    using time_point = std::chrono::time_point< test_clock, duration >;

    static constexpr bool is_steady = true;

    static time_point now() noexcept
    {
        rep ticks = static_cast< rep >(boost::winapi::GetTickCount64());
        return time_point(duration(ticks));
    }
};

#else
using test_clock = std::chrono::steady_clock;
#endif

#endif // BOOST_ATOMIC_TEST_TEST_CLOCK_HPP_INCLUDED_
