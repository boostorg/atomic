/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_gcc_atomic.hpp
 *
 * This header contains implementation of the \c operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_GCC_ATOMIC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_GCC_ATOMIC_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_types.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

BOOST_FORCEINLINE BOOST_CONSTEXPR int convert_memory_order_to_gcc(memory_order order) BOOST_NOEXCEPT
{
    return (order == memory_order_relaxed ? __ATOMIC_RELAXED : (order == memory_order_consume ? __ATOMIC_CONSUME :
        (order == memory_order_acquire ? __ATOMIC_ACQUIRE : (order == memory_order_release ? __ATOMIC_RELEASE :
        (order == memory_order_acq_rel ? __ATOMIC_ACQ_REL : __ATOMIC_SEQ_CST)))));
}

template< typename T >
struct gcc_atomic_operations
{
    typedef T storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        __atomic_store_n(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return __atomic_load_n(&storage, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return __atomic_fetch_add(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return __atomic_fetch_sub(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return __atomic_exchange_n(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return __atomic_compare_exchange_n
        (
            &storage, &expected, desired, false,
            atomics::detail::convert_memory_order_to_gcc(success_order),
            atomics::detail::convert_memory_order_to_gcc(failure_order)
        );
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return __atomic_compare_exchange_n
        (
            &storage, &expected, desired, true,
            atomics::detail::convert_memory_order_to_gcc(success_order),
            atomics::detail::convert_memory_order_to_gcc(failure_order)
        );
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return __atomic_fetch_and(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return __atomic_fetch_or(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return __atomic_fetch_xor(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        return __atomic_test_and_set(&storage, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order = memory_order_seq_cst) BOOST_NOEXCEPT
    {
        __atomic_clear(const_cast< storage_type* >(&storage), atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile& storage) BOOST_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(storage_type), &storage);
    }
};

#if BOOST_ATOMIC_INT8_LOCK_FREE > 0
template< >
struct operations< 1u > :
    public gcc_atomic_operations< storage8_t >
{
};
#endif

#if BOOST_ATOMIC_INT16_LOCK_FREE > 0
template< >
struct operations< 2u > :
    public gcc_atomic_operations< storage16_t >
{
};
#endif

#if BOOST_ATOMIC_INT32_LOCK_FREE > 0
template< >
struct operations< 4u > :
    public gcc_atomic_operations< storage32_t >
{
};
#endif

#if BOOST_ATOMIC_INT64_LOCK_FREE > 0
template< >
struct operations< 8u > :
    public gcc_atomic_operations< storage64_t >
{
};
#endif

#if BOOST_ATOMIC_INT128_LOCK_FREE > 0
template< >
struct operations< 16u > :
    public gcc_atomic_operations< storage128_t >
{
};
#endif

} // namespace detail

BOOST_FORCEINLINE void atomic_thread_fence(memory_order order)
{
    __atomic_thread_fence(atomics::detail::convert_memory_order_to_gcc(order));
}

BOOST_FORCEINLINE void atomic_signal_fence(memory_order order)
{
    __atomic_signal_fence(atomics::detail::convert_memory_order_to_gcc(order));
}

} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_ATOMIC_HPP_INCLUDED_
