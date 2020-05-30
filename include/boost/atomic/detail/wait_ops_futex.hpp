/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_ops_futex.hpp
 *
 * This header contains implementation of the wait/notify atomic operations based on futexes.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_OPS_FUTEX_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_OPS_FUTEX_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/futex.hpp>
#include <boost/atomic/detail/wait_operations_fwd.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename Base >
struct wait_operations< Base, 4u, true > :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type wait(storage_type const volatile& storage, storage_type old_val, memory_order order) BOOST_NOEXCEPT
    {
        storage_type new_val = base_type::load(storage, order);
        while (new_val == old_val)
        {
            atomics::detail::futex_wait_private(const_cast< storage_type* >(&storage), old_val);
            new_val = base_type::load(storage, order);
        }

        return new_val;
    }

    static BOOST_FORCEINLINE void notify_one(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        atomics::detail::futex_signal_private(const_cast< storage_type* >(&storage));
    }

    static BOOST_FORCEINLINE void notify_all(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        atomics::detail::futex_broadcast_private(const_cast< storage_type* >(&storage));
    }
};

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_WAIT_OPS_FUTEX_HPP_INCLUDED_
