/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_emulated.hpp
 *
 * This header contains lockpool-based implementation of the \c operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_EMULATED_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_EMULATED_HPP_INCLUDED_

#include <cstring>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_types.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/detail/lockpool.hpp>
#include <boost/atomic/capabilities.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename T >
struct emulated_operations
{
    typedef T storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        lockpool::scoped_lock lock(&storage);
        const_cast< storage_type& >(storage) = v;
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        lockpool::scoped_lock lock(&storage);
        return const_cast< storage_type const& >(storage);
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        storage_type& s = const_cast< storage_type& >(storage);
        lockpool::scoped_lock lock(&storage);
        storage_type old_val = s;
        s += v;
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        storage_type& s = const_cast< storage_type& >(storage);
        lockpool::scoped_lock lock(&storage);
        storage_type old_val = s;
        s -= v;
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        storage_type& s = const_cast< storage_type& >(storage);
        lockpool::scoped_lock lock(&storage);
        storage_type old_val = s;
        s = v;
        return old_val;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type& s = const_cast< storage_type& >(storage);
        lockpool::scoped_lock lock(&storage);
        storage_type old_val = s;
        const bool res = old_val == expected;
        if (res)
            s = desired;
        expected = old_val;

        return res;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        storage_type& s = const_cast< storage_type& >(storage);
        lockpool::scoped_lock lock(&storage);
        storage_type old_val = s;
        s &= v;
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        storage_type& s = const_cast< storage_type& >(storage);
        lockpool::scoped_lock lock(&storage);
        storage_type old_val = s;
        s |= v;
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        storage_type& s = const_cast< storage_type& >(storage);
        lockpool::scoped_lock lock(&storage);
        storage_type old_val = s;
        s ^= v;
        return old_val;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return exchange(storage, (storage_type)1, order) != (storage_type)0;
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return false;
    }
};

template< unsigned int Size >
struct storage_t
{
    unsigned char data[Size];

    bool operator== (storage_t const& that) const
    {
        return std::memcmp(data, that.data, Size) == 0;
    }
    bool operator!= (storage_t const& that) const
    {
        return std::memcmp(data, that.data, Size) != 0;
    }
};

template< unsigned int Size >
struct default_storage_type
{
    typedef storage_t< Size > type;
};

template< >
struct default_storage_type< 1u >
{
    typedef storage8_t type;
};

template< >
struct default_storage_type< 2u >
{
    typedef storage16_t type;
};

template< >
struct default_storage_type< 4u >
{
    typedef storage32_t type;
};

template< >
struct default_storage_type< 8u >
{
    typedef storage64_t type;
};

template< >
struct default_storage_type< 16u >
{
    typedef storage128_t type;
};

template< unsigned int Size >
struct operations :
    public emulated_operations< typename default_storage_type< Size >::type >
{
};

} // namespace detail

#if BOOST_ATOMIC_THREAD_FENCE == 0
BOOST_FORCEINLINE void atomic_thread_fence(memory_order)
{
    // Emulate full fence by locking/unlocking a mutex
    detail::lockpool::scoped_lock lock(0);
}
#endif

#if BOOST_ATOMIC_SIGNAL_FENCE == 0
BOOST_FORCEINLINE void atomic_signal_fence(memory_order)
{
    // We can't use pthread functions in signal handlers, so only use lock pool if it is based on atomic_flags.
    // However, any reasonable backend with a lockfree atomic_flag should provide fence primitives already.
    // So this condition is more for completeness sake.
#if BOOST_ATOMIC_FLAG_LOCK_FREE == 2
    detail::lockpool::scoped_lock lock(0);
#endif
}
#endif

} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_EMULATED_HPP_INCLUDED_
