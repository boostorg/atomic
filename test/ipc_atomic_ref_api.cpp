//  Copyright (c) 2020 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic/ipc_atomic_ref.hpp>
#include <boost/atomic/capabilities.hpp>

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include "api_test_helpers.hpp"

int main(int, char *[])
{
#if BOOST_ATOMIC_INT8_LOCK_FREE == 2
    test_integral_api< ipc_atomic_ref_wrapper, boost::uint8_t >();
    test_integral_api< ipc_atomic_ref_wrapper, boost::int8_t >();
#endif
#if BOOST_ATOMIC_INT16_LOCK_FREE == 2
    test_integral_api< ipc_atomic_ref_wrapper, boost::uint16_t >();
    test_integral_api< ipc_atomic_ref_wrapper, boost::int16_t >();
#endif
#if BOOST_ATOMIC_INT32_LOCK_FREE == 2
    test_integral_api< ipc_atomic_ref_wrapper, boost::uint32_t >();
    test_integral_api< ipc_atomic_ref_wrapper, boost::int32_t >();
#endif
#if BOOST_ATOMIC_INT64_LOCK_FREE == 2
    test_integral_api< ipc_atomic_ref_wrapper, boost::uint64_t >();
    test_integral_api< ipc_atomic_ref_wrapper, boost::int64_t >();
#endif
#if defined(BOOST_HAS_INT128) && !defined(BOOST_ATOMIC_TESTS_NO_INT128) && BOOST_ATOMIC_INT128_LOCK_FREE == 2
    test_integral_api< ipc_atomic_ref_wrapper, boost::int128_type >();
    test_integral_api< ipc_atomic_ref_wrapper, boost::uint128_type >();
#endif

#if !defined(BOOST_ATOMIC_NO_FLOATING_POINT)
#if BOOST_ATOMIC_FLOAT_LOCK_FREE == 2
    test_floating_point_api< ipc_atomic_ref_wrapper, float >();
#endif
#if BOOST_ATOMIC_DOUBLE_LOCK_FREE == 2
    test_floating_point_api< ipc_atomic_ref_wrapper, double >();
#endif
#if BOOST_ATOMIC_LONG_DOUBLE_LOCK_FREE == 2
    test_floating_point_api< ipc_atomic_ref_wrapper, long double >();
#endif
#if defined(BOOST_HAS_FLOAT128) && !defined(BOOST_ATOMIC_TESTS_NO_FLOAT128) && BOOST_ATOMIC_FLOAT128_LOCK_FREE == 2
    test_floating_point_api< ipc_atomic_ref_wrapper, boost::float128_type >();
#endif
#endif

#if BOOST_ATOMIC_POINTER_LOCK_FREE == 2
    test_pointer_api< ipc_atomic_ref_wrapper, int >();
#endif

#if BOOST_ATOMIC_INT_LOCK_FREE == 2
    test_enum_api< ipc_atomic_ref_wrapper >();
#endif

    return boost::report_errors();
}
