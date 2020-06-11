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

#include <cstddef>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/capabilities.hpp>
#if defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B) || defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)
#include <boost/atomic/detail/ops_gcc_x86_dcas.hpp>
#include <boost/atomic/detail/ops_cas_based.hpp>
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

struct gcc_x86_operations_base
{
    static BOOST_CONSTEXPR_OR_CONST bool full_cas_based = false;
    static BOOST_CONSTEXPR_OR_CONST bool is_always_lock_free = true;

    static BOOST_FORCEINLINE void fence_before(memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & static_cast< unsigned int >(memory_order_release)) != 0u)
            __asm__ __volatile__ ("" ::: "memory");
    }

    static BOOST_FORCEINLINE void fence_after(memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_consume) | static_cast< unsigned int >(memory_order_acquire))) != 0u)
            __asm__ __volatile__ ("" ::: "memory");
    }
};

template< std::size_t Size, bool Signed, bool Interprocess, typename Derived >
struct gcc_x86_operations :
    public gcc_x86_operations_base
{
    typedef typename storage_traits< Size >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = Size;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = Size;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

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
};

template< bool Signed, bool Interprocess >
struct operations< 1u, Signed, Interprocess > :
    public gcc_x86_operations< 1u, Signed, Interprocess, operations< 1u, Signed, Interprocess > >
{
    typedef gcc_x86_operations< 1u, Signed, Interprocess, operations< 1u, Signed, Interprocess > > base_type;
    typedef typename base_type::storage_type storage_type;
    typedef typename storage_traits< 4u >::type temp_storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "lock; xaddb %0, %1"
            : "+q" (v), "+m" (storage)
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
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
#if defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchgb %3, %1"
            : "+a" (previous), "+m" (storage), "=@ccz" (success)
            : "q" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#else // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchgb %3, %1\n\t"
            "sete %2"
            : "+a" (previous), "+m" (storage), "=q" (success)
            : "q" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#endif // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        expected = previous;
        return success;
    }

#define BOOST_ATOMIC_DETAIL_CAS_LOOP(op, argument, result)\
    temp_storage_type new_val;\
    __asm__ __volatile__\
    (\
        ".align 16\n\t"\
        "1: mov %[arg], %2\n\t"\
        op " %%al, %b2\n\t"\
        "lock; cmpxchgb %b2, %[storage]\n\t"\
        "jne 1b"\
        : [res] "+a" (result), [storage] "+m" (storage), "=&q" (new_val)\
        : [arg] "ir" ((temp_storage_type)argument)\
        : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
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

template< bool Signed, bool Interprocess >
struct operations< 2u, Signed, Interprocess > :
    public gcc_x86_operations< 2u, Signed, Interprocess, operations< 2u, Signed, Interprocess > >
{
    typedef gcc_x86_operations< 2u, Signed, Interprocess, operations< 2u, Signed, Interprocess > > base_type;
    typedef typename base_type::storage_type storage_type;
    typedef typename storage_traits< 4u >::type temp_storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "lock; xaddw %0, %1"
            : "+q" (v), "+m" (storage)
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
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
#if defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchgw %3, %1"
            : "+a" (previous), "+m" (storage), "=@ccz" (success)
            : "q" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#else // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchgw %3, %1\n\t"
            "sete %2"
            : "+a" (previous), "+m" (storage), "=q" (success)
            : "q" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#endif // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        expected = previous;
        return success;
    }

#define BOOST_ATOMIC_DETAIL_CAS_LOOP(op, argument, result)\
    temp_storage_type new_val;\
    __asm__ __volatile__\
    (\
        ".align 16\n\t"\
        "1: mov %[arg], %2\n\t"\
        op " %%ax, %w2\n\t"\
        "lock; cmpxchgw %w2, %[storage]\n\t"\
        "jne 1b"\
        : [res] "+a" (result), [storage] "+m" (storage), "=&q" (new_val)\
        : [arg] "ir" ((temp_storage_type)argument)\
        : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
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

template< bool Signed, bool Interprocess >
struct operations< 4u, Signed, Interprocess > :
    public gcc_x86_operations< 4u, Signed, Interprocess, operations< 4u, Signed, Interprocess > >
{
    typedef gcc_x86_operations< 4u, Signed, Interprocess, operations< 4u, Signed, Interprocess > > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "lock; xaddl %0, %1"
            : "+r" (v), "+m" (storage)
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
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
#if defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchgl %3, %1"
            : "+a" (previous), "+m" (storage), "=@ccz" (success)
            : "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#else // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchgl %3, %1\n\t"
            "sete %2"
            : "+a" (previous), "+m" (storage), "=q" (success)
            : "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#endif // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        expected = previous;
        return success;
    }

#define BOOST_ATOMIC_DETAIL_CAS_LOOP(op, argument, result)\
    storage_type new_val;\
    __asm__ __volatile__\
    (\
        ".align 16\n\t"\
        "1: mov %[arg], %[new_val]\n\t"\
        op " %%eax, %[new_val]\n\t"\
        "lock; cmpxchgl %[new_val], %[storage]\n\t"\
        "jne 1b"\
        : [res] "+a" (result), [storage] "+m" (storage), [new_val] "=&r" (new_val)\
        : [arg] "ir" (argument)\
        : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
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

template< bool Signed, bool Interprocess >
struct operations< 8u, Signed, Interprocess > :
    public cas_based_operations< gcc_dcas_x86< Signed, Interprocess > >
{
};

#elif defined(__x86_64__)

template< bool Signed, bool Interprocess >
struct operations< 8u, Signed, Interprocess > :
    public gcc_x86_operations< 8u, Signed, Interprocess, operations< 8u, Signed, Interprocess > >
{
    typedef gcc_x86_operations< 8u, Signed, Interprocess, operations< 8u, Signed, Interprocess > > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "lock; xaddq %0, %1"
            : "+r" (v), "+m" (storage)
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
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
#if defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchgq %3, %1"
            : "+a" (previous), "+m" (storage), "=@ccz" (success)
            : "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#else // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchgq %3, %1\n\t"
            "sete %2"
            : "+a" (previous), "+m" (storage), "=q" (success)
            : "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#endif // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        expected = previous;
        return success;
    }

#define BOOST_ATOMIC_DETAIL_CAS_LOOP(op, argument, result)\
    storage_type new_val;\
    __asm__ __volatile__\
    (\
        ".align 16\n\t"\
        "1: movq %[arg], %[new_val]\n\t"\
        op " %%rax, %[new_val]\n\t"\
        "lock; cmpxchgq %[new_val], %[storage]\n\t"\
        "jne 1b"\
        : [res] "+a" (result), [storage] "+m" (storage), [new_val] "=&r" (new_val)\
        : [arg] "r" (argument)\
        : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
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

template< bool Signed, bool Interprocess >
struct operations< 16u, Signed, Interprocess > :
    public cas_based_operations< gcc_dcas_x86_64< Signed, Interprocess > >
{
};

#endif // defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)

BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    if (order == memory_order_seq_cst)
    {
        // We could generate mfence for a seq_cst fence here, but a dummy lock-prefixed instruction is be enough
        // and is faster than mfence on most modern x86 CPUs (as of 2020).
        // Note that we want to apply the atomic operation on any location so that:
        // - It is not shared with other threads. A variable on the stack suits this well.
        // - It is likely in cache. Being close to the stack top fits this well.
        // - It does not alias existing data on the stack, so that we don't introduce a false data dependency.
        // See here: https://shipilev.net/blog/2014/on-the-fence-with-dependencies/
        unsigned int dummy;
        __asm__ __volatile__ ("lock; orl $0, %0" : "=m" (dummy) : : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory");
    }
    else if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_acquire) | static_cast< unsigned int >(memory_order_release))) != 0u)
    {
        __asm__ __volatile__ ("" ::: "memory");
    }
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    if (order != memory_order_relaxed)
        __asm__ __volatile__ ("" ::: "memory");
}

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_X86_HPP_INCLUDED_
