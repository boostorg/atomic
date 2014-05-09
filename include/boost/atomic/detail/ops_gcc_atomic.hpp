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
#include <boost/atomic/capabilities.hpp>
#if defined(__clang__) && (defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B) || defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B))
#include <boost/cstdint.hpp>
#include <boost/atomic/detail/ops_cas_based.hpp>
#endif

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

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        __atomic_store_n(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_load_n(&storage, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_add(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_sub(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
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

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_and(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_or(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_xor(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_test_and_set(&storage, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        __atomic_clear(const_cast< storage_type* >(&storage), atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile& storage) BOOST_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(storage_type), &storage);
    }
};

#if BOOST_ATOMIC_INT8_LOCK_FREE > 0
template< bool Signed >
struct operations< 1u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 1u, Signed >::type >
{
};
#endif

#if BOOST_ATOMIC_INT16_LOCK_FREE > 0
template< bool Signed >
struct operations< 2u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 2u, Signed >::type >
{
};
#endif

#if BOOST_ATOMIC_INT32_LOCK_FREE > 0
template< bool Signed >
struct operations< 4u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 4u, Signed >::type >
{
};
#endif

#if BOOST_ATOMIC_INT64_LOCK_FREE > 0
#if defined(__clang__) && defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B)

// Workaround for clang bug http://llvm.org/bugs/show_bug.cgi?id=19355
template< bool Signed >
struct clang_dcas_x86
{
    typedef typename make_storage_type< 8u, Signed >::type storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        if ((((uint32_t)&storage) & 0x00000007) == 0)
        {
#if defined(__SSE2__)
            __asm__ __volatile__
            (
#if defined(__AVX__)
                "vmovq %1, %%xmm4\n\t"
                "vmovq %%xmm4, %0\n\t"
#else
                "movq %1, %%xmm4\n\t"
                "movq %%xmm4, %0\n\t"
#endif
                : "=m" (storage)
                : "m" (v)
                : "memory", "xmm4"
            );
#else
            __asm__ __volatile__
            (
                "fildll %1\n\t"
                "fistpll %0\n\t"
                : "=m" (storage)
                : "m" (v)
                : "memory"
            );
#endif
        }
        else
        {
            uint32_t scratch;
            __asm__ __volatile__
            (
                "movl %%ebx, %[scratch]\n\t"
                "movl %[value_lo], %%ebx\n\t"
                "movl 0(%[dest]), %%eax\n\t"
                "movl 4(%[dest]), %%edx\n\t"
                ".align 16\n\t"
                "1: lock; cmpxchg8b 0(%[dest])\n\t"
                "jne 1b\n\t"
                "movl %[scratch], %%ebx"
                : [scratch] "=m,m" (scratch)
                : [value_lo] "a,a" ((uint32_t)v), "c,c" ((uint32_t)(v >> 32)), [dest] "D,S" (&storage)
                : "memory", "cc", "edx"
            );
        }
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type value;

        if ((((uint32_t)&storage) & 0x00000007) == 0)
        {
#if defined(__SSE2__)
            __asm__ __volatile__
            (
#if defined(__AVX__)
                "vmovq %1, %%xmm4\n\t"
                "vmovq %%xmm4, %0\n\t"
#else
                "movq %1, %%xmm4\n\t"
                "movq %%xmm4, %0\n\t"
#endif
                : "=m" (value)
                : "m" (storage)
                : "memory", "xmm4"
            );
#else
            __asm__ __volatile__
            (
                "fildll %1\n\t"
                "fistpll %0\n\t"
                : "=m" (value)
                : "m" (storage)
                : "memory"
            );
#endif
        }
        else
        {
            // We don't care for comparison result here; the previous value will be stored into value anyway.
            value = __sync_val_compare_and_swap(&storage, (storage_type)0, (storage_type)0);
        }

        return value;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type old_expected = expected;
        expected = __sync_val_compare_and_swap(&storage, old_expected, desired);
        return expected == old_expected;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }
};

template< bool Signed >
struct operations< 8u, Signed > :
    public cas_based_operations< clang_dcas_x86< Signed > >
{
};

#else

template< bool Signed >
struct operations< 8u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 8u, Signed >::type >
{
};

#endif
#endif

#if BOOST_ATOMIC_INT128_LOCK_FREE > 0
#if defined(__clang__) && defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)

// Workaround for clang bug: http://llvm.org/bugs/show_bug.cgi?id=19149
// Clang 3.4 does not implement 128-bit __atomic* intrinsics even though it defines __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
template< bool Signed >
struct clang_dcas_x86_64
{
    typedef typename make_storage_type< 16u, Signed >::type storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        uint64_t const* p_value = (uint64_t const*)&value;
        __asm__ __volatile__
        (
            "movq 0(%[dest]), %%rax\n\t"
            "movq 8(%[dest]), %%rdx\n\t"
            ".align 16\n\t"
            "1: lock; cmpxchg16b 0(%[dest])\n\t"
            "jne 1b"
            :
            : "b" (p_value[0]), "c" (p_value[1]), [dest] "r" (&storage)
            : "memory", "cc", "rax", "rdx"
        );
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type value = storage_type();
        return __sync_val_compare_and_swap(&storage, value, value);
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type old_expected = expected;
        expected = __sync_val_compare_and_swap(&storage, old_expected, desired);
        return expected == old_expected;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }
};

template< bool Signed >
struct operations< 16u, Signed > :
    public cas_based_operations< clang_dcas_x86_64< Signed > >
{
};

#else

template< bool Signed >
struct operations< 16u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 16u, Signed >::type >
{
};

#endif
#endif

BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    __atomic_thread_fence(atomics::detail::convert_memory_order_to_gcc(order));
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    __atomic_signal_fence(atomics::detail::convert_memory_order_to_gcc(order));
}

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_ATOMIC_HPP_INCLUDED_
