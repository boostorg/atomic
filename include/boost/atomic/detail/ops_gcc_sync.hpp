/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2011 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_gcc_sync.hpp
 *
 * This header contains implementation of the \c operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_GCC_SYNC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_GCC_SYNC_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_types.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/capabilities.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename T >
struct gcc_sync_operations
{
    typedef T storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before_store(order);
        storage = v;
        fence_after_store(order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v = storage;
        fence_after_load(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_add(&storage, v);
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_sub(&storage, v);
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        // GCC docs mention that not all architectures may support full exchange semantics for this intrinsic. However, GCC's implementation of
        // std::atomic<> uses this intrinsic unconditionally. We do so as well. In case if some architectures actually don't support this, we can always
        // add a check here and fall back to a CAS loop.
        if ((order & memory_order_release) != 0)
            __sync_synchronize();
        return __sync_lock_test_and_set(&storage, v);
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type expected2 = expected;
        storage_type old_val = __sync_val_compare_and_swap(&storage, expected2, desired);

        if (old_val == expected2)
        {
            return true;
        }
        else
        {
            expected = old_val;
            return false;
        }
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_and(&storage, v);
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_or(&storage, v);
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_xor(&storage, v);
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        if ((order & memory_order_release) != 0)
            __sync_synchronize();
        return __sync_lock_test_and_set(&storage, 1);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        __sync_lock_release(&storage);
        if (order == memory_order_seq_cst)
            __sync_synchronize();
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }

private:
    static BOOST_FORCEINLINE void fence_before_store(memory_order order) BOOST_NOEXCEPT
    {
        switch (order)
        {
        case memory_order_relaxed:
        case memory_order_acquire:
        case memory_order_consume:
            break;
        case memory_order_release:
        case memory_order_acq_rel:
        case memory_order_seq_cst:
            __sync_synchronize();
            break;
        }
    }

    static BOOST_FORCEINLINE void fence_after_store(memory_order order) BOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
            __sync_synchronize();
    }

    static BOOST_FORCEINLINE void fence_after_load(memory_order order) BOOST_NOEXCEPT
    {
        switch (order)
        {
        case memory_order_relaxed:
        case memory_order_release:
            break;
        case memory_order_consume:
        case memory_order_acquire:
        case memory_order_acq_rel:
        case memory_order_seq_cst:
            __sync_synchronize();
            break;
        }
    }
};

#if BOOST_ATOMIC_INT8_LOCK_FREE > 0
template< >
struct operations< 1u > :
    public gcc_sync_operations<
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1)
        storage8_t
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2)
        storage16_t
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
        storage32_t
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
        storage64_t
#else
        storage128_t
#endif
    >
{
};
#endif

#if BOOST_ATOMIC_INT16_LOCK_FREE > 0
template< >
struct operations< 2u > :
    public gcc_sync_operations<
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2)
        storage16_t
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
        storage32_t
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
        storage64_t
#else
        storage128_t
#endif
    >
{
};
#endif

#if BOOST_ATOMIC_INT32_LOCK_FREE > 0
template< >
struct operations< 4u > :
    public gcc_sync_operations<
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
        storage32_t
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
        storage64_t
#else
        storage128_t
#endif
    >
{
};
#endif

#if BOOST_ATOMIC_INT64_LOCK_FREE > 0
template< >
struct operations< 8u > :
    public gcc_sync_operations<
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
        storage64_t
#else
        storage128_t
#endif
    >
{
};

#endif

#if BOOST_ATOMIC_INT128_LOCK_FREE > 0
template< >
struct operations< 16u > :
    public gcc_sync_operations< storage128_t >
{
};
#endif

BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    switch (order)
    {
    case memory_order_relaxed:
        break;
    case memory_order_release:
    case memory_order_consume:
    case memory_order_acquire:
    case memory_order_acq_rel:
    case memory_order_seq_cst:
        __sync_synchronize();
        break;
    }
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    switch (order)
    {
    case memory_order_relaxed:
    case memory_order_consume:
        break;
    case memory_order_acquire:
    case memory_order_release:
    case memory_order_acq_rel:
    case memory_order_seq_cst:
        __asm__ __volatile__ ("" ::: "memory");
        break;
    default:;
    }
}

} // namespace detail

} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_SYNC_HPP_INCLUDED_
