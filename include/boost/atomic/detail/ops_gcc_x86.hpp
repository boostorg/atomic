/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2012 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_gcc_x86.hpp
 *
 * This header contains implementation of the \c operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_GCC_X86_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_GCC_X86_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_type.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/capabilities.hpp>
#if defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B) || defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)
#include <boost/cstdint.hpp>
#include <boost/atomic/detail/ops_cas_based.hpp>
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__x86_64__)
#define BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER "rdx"
#else
#define BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER "edx"
#endif

namespace boost {
namespace atomics {
namespace detail {

struct gcc_x86_operations_base
{
    static BOOST_FORCEINLINE void fence_before(memory_order order) BOOST_NOEXCEPT
    {
        if ((order & memory_order_release) != 0)
            __asm__ __volatile__ ("" ::: "memory");
    }

    static BOOST_FORCEINLINE void fence_after(memory_order order) BOOST_NOEXCEPT
    {
        if ((order & memory_order_acquire) != 0)
            __asm__ __volatile__ ("" ::: "memory");
    }
};

template< typename T, typename Derived >
struct gcc_x86_operations :
    public gcc_x86_operations_base
{
    typedef T storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_seq_cst)
        {
            fence_before(order);
            storage = v;
            fence_after(order);
        }
        else
        {
            Derived::exchange(storage, v, order);
        }
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v = storage;
        fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return Derived::fetch_add(storage, -v, order);
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return Derived::compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!Derived::exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }
};

template< bool Signed >
struct operations< 1u, Signed > :
    public gcc_x86_operations< typename make_storage_type< 1u, Signed >::type, operations< 1u, Signed > >
{
    typedef gcc_x86_operations< typename make_storage_type< 1u, Signed >::type, operations< 1u, Signed > > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "lock; xaddb %0, %1"
            : "+q" (v), "+m" (storage)
            :
            : "cc", "memory"
        );
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "xchgb %0, %1"
            : "+q" (v), "+m" (storage)
            :
            : "memory"
        );
        return v;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type previous = expected;
        bool success;
        __asm__ __volatile__
        (
            "lock; cmpxchgb %3, %1\n\t"
            "sete %2"
            : "+a" (previous), "+m" (storage), "=q" (success)
            : "q" (desired)
            : "cc", "memory"
        );
        expected = previous;
        return success;
    }

#define BOOST_ATOMIC_DETAIL_CAS_LOOP(op, argument, result)\
    __asm__ __volatile__\
    (\
        "xor %%" BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER ", %%" BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER "\n\t"\
        ".align 16\n\t"\
        "1: movb %[arg], %%dl\n\t"\
        op " %%al, %%dl\n\t"\
        "lock; cmpxchgb %%dl, %[storage]\n\t"\
        "jne 1b"\
        : [res] "+a" (result), [storage] "+m" (storage)\
        : [arg] "q" (argument)\
        : "cc", BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER, "memory"\
    )

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("andb", v, res);
        return res;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("orb", v, res);
        return res;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("xorb", v, res);
        return res;
    }

#undef BOOST_ATOMIC_DETAIL_CAS_LOOP
};

template< bool Signed >
struct operations< 2u, Signed > :
    public gcc_x86_operations< typename make_storage_type< 2u, Signed >::type, operations< 2u, Signed > >
{
    typedef gcc_x86_operations< typename make_storage_type< 2u, Signed >::type, operations< 2u, Signed > > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "lock; xaddw %0, %1"
            : "+q" (v), "+m" (storage)
            :
            : "cc", "memory"
        );
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "xchgw %0, %1"
            : "+q" (v), "+m" (storage)
            :
            : "memory"
        );
        return v;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type previous = expected;
        bool success;
        __asm__ __volatile__
        (
            "lock; cmpxchgw %3, %1\n\t"
            "sete %2"
            : "+a" (previous), "+m" (storage), "=q" (success)
            : "q" (desired)
            : "cc", "memory"
        );
        expected = previous;
        return success;
    }

#define BOOST_ATOMIC_DETAIL_CAS_LOOP(op, argument, result)\
    __asm__ __volatile__\
    (\
        "xor %%" BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER ", %%" BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER "\n\t"\
        ".align 16\n\t"\
        "1: movw %[arg], %%dx\n\t"\
        op " %%ax, %%dx\n\t"\
        "lock; cmpxchgw %%dx, %[storage]\n\t"\
        "jne 1b"\
        : [res] "+a" (result), [storage] "+m" (storage)\
        : [arg] "q" (argument)\
        : "cc", BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER, "memory"\
    )

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("andw", v, res);
        return res;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("orw", v, res);
        return res;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("xorw", v, res);
        return res;
    }

#undef BOOST_ATOMIC_DETAIL_CAS_LOOP
};

template< bool Signed >
struct operations< 4u, Signed > :
    public gcc_x86_operations< typename make_storage_type< 4u, Signed >::type, operations< 4u, Signed > >
{
    typedef gcc_x86_operations< typename make_storage_type< 4u, Signed >::type, operations< 4u, Signed > > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "lock; xaddl %0, %1"
            : "+r" (v), "+m" (storage)
            :
            : "cc", "memory"
        );
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "xchgl %0, %1"
            : "+r" (v), "+m" (storage)
            :
            : "memory"
        );
        return v;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type previous = expected;
        bool success;
        __asm__ __volatile__
        (
            "lock; cmpxchgl %3, %1\n\t"
            "sete %2"
            : "+a" (previous), "+m" (storage), "=q" (success)
            : "r" (desired)
            : "cc", "memory"
        );
        expected = previous;
        return success;
    }

#define BOOST_ATOMIC_DETAIL_CAS_LOOP(op, argument, result)\
    __asm__ __volatile__\
    (\
        "xor %%" BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER ", %%" BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER "\n\t"\
        ".align 16\n\t"\
        "1: movl %[arg], %%edx\n\t"\
        op " %%eax, %%edx\n\t"\
        "lock; cmpxchgl %%edx, %[storage]\n\t"\
        "jne 1b"\
        : [res] "+a" (result), [storage] "+m" (storage)\
        : [arg] "r" (argument)\
        : "cc", BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER, "memory"\
    )

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("andl", v, res);
        return res;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("orl", v, res);
        return res;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("xorl", v, res);
        return res;
    }

#undef BOOST_ATOMIC_DETAIL_CAS_LOOP
};

#if defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B)

template< bool Signed >
struct gcc_dcas_x86
{
    typedef typename make_storage_type< 8u, Signed >::type storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
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
                : "cc", "edx", "memory"
            );
        }
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order) BOOST_NOEXCEPT
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
            // Also we don't care for ebx and ecx values, they just have to be equal to eax and edx before cmpxchg8b.
            __asm__ __volatile__
            (
                "movl %%ebx, %%eax\n\t"
                "movl %%ecx, %%edx\n\t"
                "lock; cmpxchg8b %[storage]"
                : "=&A" (value)
                : [storage] "m" (storage)
                : "cc", "memory"
            );
        }

        return value;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
#if defined(__PIC__)
        // Make sure ebx is saved and restored properly in case
        // of position independent code. To make this work
        // setup register constraints such that ebx can not be
        // used by accident e.g. as base address for the variable
        // to be modified. Accessing "scratch" should always be okay,
        // as it can only be placed on the stack (and therefore
        // accessed through ebp or esp only).
        //
        // In theory, could push/pop ebx onto/off the stack, but movs
        // to a prepared stack slot turn out to be faster.

        uint32_t scratch;
        bool success;
        __asm__ __volatile__
        (
            "movl %%ebx, %[scratch]\n\t"
            "movl %[desired_lo], %%ebx\n\t"
            "lock; cmpxchg8b %[dest]\n\t"
            "movl %[scratch], %%ebx\n\t"
            "sete %[success]"
            : "+A,A,A,A,A,A" (expected), [dest] "+m,m,m,m,m,m" (storage), [scratch] "=m,m,m,m,m,m" (scratch), [success] "=q,m,q,m,q,m" (success)
            : [desired_lo] "S,S,D,D,m,m" ((uint32_t)desired), "c,c,c,c,c,c" ((uint32_t)(desired >> 32))
            : "cc", "memory"
        );
        return success;
#else
        bool success;
        __asm__ __volatile__
        (
            "lock; cmpxchg8b %[dest]\n\t"
            "sete %[success]"
            : "+A,A" (expected), [dest] "+m,m" (storage), [scratch] "=m,m" (scratch), [success] "=q,m" (success)
            : "b,b" ((uint32_t)desired), "c,c" ((uint32_t)(desired >> 32))
            : "cc", "memory"
        );
        return success;
#endif
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
    public cas_based_operations< gcc_dcas_x86< Signed > >
{
};

#elif defined(__x86_64__)

template< bool Signed >
struct operations< 8u, Signed > :
    public gcc_x86_operations< typename make_storage_type< 8u, Signed >::type, operations< 8u, Signed > >
{
    typedef gcc_x86_operations< typename make_storage_type< 8u, Signed >::type, operations< 8u, Signed > > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "lock; xaddq %0, %1"
            : "+r" (v), "+m" (storage)
            :
            : "cc", "memory"
        );
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "xchgq %0, %1"
            : "+r" (v), "+m" (storage)
            :
            : "memory"
        );
        return v;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type previous = expected;
        bool success;
        __asm__ __volatile__
        (
            "lock; cmpxchgq %3, %1\n\t"
            "sete %2"
            : "+a" (previous), "+m" (storage), "=q" (success)
            : "r" (desired)
            : "cc", "memory"
        );
        expected = previous;
        return success;
    }

#define BOOST_ATOMIC_DETAIL_CAS_LOOP(op, argument, result)\
    __asm__ __volatile__\
    (\
        "xor %%" BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER ", %%" BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER "\n\t"\
        ".align 16\n\t"\
        "1: movq %[arg], %%rdx\n\t"\
        op " %%rax, %%rdx\n\t"\
        "lock; cmpxchgq %%rdx, %[storage]\n\t"\
        "jne 1b"\
        : [res] "+a" (result), [storage] "+m" (storage)\
        : [arg] "r" (argument)\
        : "cc", BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER, "memory"\
    )

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("andq", v, res);
        return res;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("orq", v, res);
        return res;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        storage_type res = storage;
        BOOST_ATOMIC_DETAIL_CAS_LOOP("xorq", v, res);
        return res;
    }

#undef BOOST_ATOMIC_DETAIL_CAS_LOOP
};

#endif

#if defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)

template< bool Signed >
struct gcc_dcas_x86_64
{
    typedef typename make_storage_type< 16u, Signed >::type storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        uint64_t const* p_value = (uint64_t const*)&v;
        __asm__ __volatile__
        (
            "movq 0(%[dest]), %%rax\n\t"
            "movq 8(%[dest]), %%rdx\n\t"
            ".align 16\n\t"
            "1: lock; cmpxchg16b 0(%[dest])\n\t"
            "jne 1b"
            :
            : "b" (p_value[0]), "c" (p_value[1]), [dest] "r" (&storage)
            : "cc", "rax", "rdx", "memory"
        );
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order) BOOST_NOEXCEPT
    {
        storage_type value;

        // We don't care for comparison result here; the previous value will be stored into value anyway.
        // Also we don't care for rbx and rcx values, they just have to be equal to rax and rdx before cmpxchg16b.
        __asm__ __volatile__
        (
            "movq %%rbx, %%rax\n\t"
            "movq %%rcx, %%rdx\n\t"
            "lock; cmpxchg16b %[storage]"
            : "=&A" (value)
            : [storage] "m" (storage)
            : "cc", "memory"
        );

        return value;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        uint64_t const* p_desired = (uint64_t const*)&desired;
        bool success;
        __asm__ __volatile__
        (
            "lock; cmpxchg16b %[dest]\n\t"
            "sete %[success]"
            : "+A,A" (expected), [dest] "+m,m" (storage), [success] "=q,m" (success)
            : "b,b" (p_desired[0]), "c,c" (p_desired[1])
            : "cc", "memory"
        );
        return success;
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
    public cas_based_operations< gcc_dcas_x86_64< Signed > >
{
};

#endif // defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)

BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    if (order == memory_order_seq_cst)
    {
        __asm__ __volatile__
        (
#if defined(__x86_64__) || defined(__SSE2__)
            "mfence\n"
#else
            "lock; addl $0, (%%esp)\n"
#endif
            ::: "memory"
        );
    }
    else if ((order & (memory_order_acquire | memory_order_release)) != 0)
    {
        __asm__ __volatile__ ("" ::: "memory");
    }
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    if ((order & ~memory_order_consume) != 0)
        __asm__ __volatile__ ("" ::: "memory");
}

} // namespace detail
} // namespace atomics
} // namespace boost

#undef BOOST_ATOMIC_DETAIL_TEMP_CAS_REGISTER

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_X86_HPP_INCLUDED_
