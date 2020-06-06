//  Copyright (c) 2020 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic/ipc_atomic_ref.hpp>
#include <boost/atomic/capabilities.hpp>

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include "ipc_wait_test_helpers.hpp"

int main(int, char *[])
{
#if BOOST_ATOMIC_INT8_LOCK_FREE == 2
    test_wait_notify_api< ipc_atomic_ref_wrapper, boost::uint8_t >(1, 2, 3);
#endif
#if BOOST_ATOMIC_INT16_LOCK_FREE == 2
    test_wait_notify_api< ipc_atomic_ref_wrapper, boost::uint16_t >(1, 2, 3);
#endif
#if BOOST_ATOMIC_INT32_LOCK_FREE == 2
    test_wait_notify_api< ipc_atomic_ref_wrapper, boost::uint32_t >(1, 2, 3);
#endif
#if BOOST_ATOMIC_INT64_LOCK_FREE == 2
    test_wait_notify_api< ipc_atomic_ref_wrapper, boost::uint64_t >(1, 2, 3);
#endif
#if defined(BOOST_HAS_INT128) && !defined(BOOST_ATOMIC_TESTS_NO_INT128) && BOOST_ATOMIC_INT128_LOCK_FREE == 2
    test_wait_notify_api< ipc_atomic_ref_wrapper, boost::uint128_type >(1, 2, 3);
#endif

    return boost::report_errors();
}
