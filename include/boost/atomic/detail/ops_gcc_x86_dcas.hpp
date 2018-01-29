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
 * \file   atomic/detail/ops_gcc_x86_dcas.hpp
 *
 * This header contains implementation of the double-width CAS primitive for x86.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_GCC_X86_DCAS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_GCC_X86_DCAS_HPP_INCLUDED_

#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_type.hpp>
#include <boost/atomic/capabilities.hpp>
#if defined(BOOST_ATOMIC_DETAIL_NO_ASM_RAX_RDX_PAIRS) && !defined(BOOST_ATOMIC_DETAIL_HAS_BUILTIN_MEMCPY)
#include <cstring>
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

#if defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B)

template< bool Signed >
struct gcc_dcas_x86
{
    typedef typename make_storage_type< 8u, Signed >::type storage_type;
    typedef typename make_storage_type< 8u, Signed >::aligned aligned_storage_type;

    static BOOST_CONSTEXPR_OR_CONST bool is_always_lock_free = true;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        if (BOOST_LIKELY((((uint32_t)&storage) & 0x00000007) == 0u))
        {
#if defined(__SSE__)
            typedef float xmm_t __attribute__((__vector_size__(16)));
            xmm_t xmm_scratch;
            __asm__ __volatile__
            (
#if defined(__AVX__)
                "vmovq %[value], %[xmm_scratch]\n\t"
                "vmovq %[xmm_scratch], %[storage]\n\t"
#elif defined(__SSE2__)
                "movq %[value], %[xmm_scratch]\n\t"
                "movq %[xmm_scratch], %[storage]\n\t"
#else
                "xorps %[xmm_scratch], %[xmm_scratch]\n\t"
                "movlps %[value], %[xmm_scratch]\n\t"
                "movlps %[xmm_scratch], %[storage]\n\t"
#endif
                : [storage] "=m" (storage), [xmm_scratch] "=x" (xmm_scratch)
                : [value] "m" (v)
                : "memory"
            );
#else
            __asm__ __volatile__
            (
                "fildll %[value]\n\t"
                "fistpll %[storage]\n\t"
                : [storage] "=m" (storage)
                : [value] "m" (v)
                : "memory"
            );
#endif
        }
        else
        {
#if defined(__PIC__)
            uint32_t scratch;
            __asm__ __volatile__
            (
                "movl %%ebx, %[scratch]\n\t"
                "movl %%eax, %%ebx\n\t"
                "movl %[dest_lo], %%eax\n\t"
                "movl %[dest_hi], %%edx\n\t"
                ".align 16\n\t"
                "1: lock; cmpxchg8b %[dest_lo]\n\t"
                "jne 1b\n\t"
                "movl %[scratch], %%ebx\n\t"
                : [scratch] "=m" (scratch), [dest_lo] "=m" (storage), [dest_hi] "=m" (reinterpret_cast< volatile uint32_t* >(&storage)[1])
                : "a" ((uint32_t)v), "c" ((uint32_t)(v >> 32))
                : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "edx", "memory"
            );
#else // defined(__PIC__)
            __asm__ __volatile__
            (
                "movl %[dest_lo], %%eax\n\t"
                "movl %[dest_hi], %%edx\n\t"
                ".align 16\n\t"
                "1: lock; cmpxchg8b %[dest_lo]\n\t"
                "jne 1b\n\t"
                : [dest_lo] "=m" (storage), [dest_hi] "=m" (reinterpret_cast< volatile uint32_t* >(&storage)[1])
                : [value_lo] "b" ((uint32_t)v), "c" ((uint32_t)(v >> 32))
                : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "eax", "edx", "memory"
            );
#endif // defined(__PIC__)
        }
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order) BOOST_NOEXCEPT
    {
        storage_type value;

        if (BOOST_LIKELY((((uint32_t)&storage) & 0x00000007) == 0u))
        {
#if defined(__SSE__)
            typedef float xmm_t __attribute__((__vector_size__(16)));
            xmm_t xmm_scratch;
            __asm__ __volatile__
            (
#if defined(__AVX__)
                "vmovq %[storage], %[xmm_scratch]\n\t"
                "vmovq %[xmm_scratch], %[value]\n\t"
#elif defined(__SSE2__)
                "movq %[storage], %[xmm_scratch]\n\t"
                "movq %[xmm_scratch], %[value]\n\t"
#else
                "xorps %[xmm_scratch], %[xmm_scratch]\n\t"
                "movlps %[storage], %[xmm_scratch]\n\t"
                "movlps %[xmm_scratch], %[value]\n\t"
#endif
                : [value] "=m" (value), [xmm_scratch] "=x" (xmm_scratch)
                : [storage] "m" (storage)
                : "memory"
            );
#else
            __asm__ __volatile__
            (
                "fildll %[storage]\n\t"
                "fistpll %[value]\n\t"
                : [value] "=m" (value)
                : [storage] "m" (storage)
                : "memory"
            );
#endif
        }
        else
        {
#if defined(__clang__)
            // Clang cannot allocate eax:edx register pairs but it has sync intrinsics
            value = __sync_val_compare_and_swap(&storage, (storage_type)0, (storage_type)0);
#else
            // We don't care for comparison result here; the previous value will be stored into value anyway.
            // Also we don't care for ebx and ecx values, they just have to be equal to eax and edx before cmpxchg8b.
            __asm__ __volatile__
            (
                "movl %%ebx, %%eax\n\t"
                "movl %%ecx, %%edx\n\t"
                "lock; cmpxchg8b %[storage]\n\t"
                : "=&A" (value)
                : [storage] "m" (storage)
                : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
            );
#endif
        }

        return value;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
#if defined(__clang__)

        // Clang cannot allocate eax:edx register pairs but it has sync intrinsics
        storage_type old_expected = expected;
        expected = __sync_val_compare_and_swap(&storage, old_expected, desired);
        return expected == old_expected;

#elif defined(__PIC__)

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
#if defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "movl %%ebx, %[scratch]\n\t"
            "movl %[desired_lo], %%ebx\n\t"
            "lock; cmpxchg8b (%[dest])\n\t"
            "movl %[scratch], %%ebx\n\t"
            : "+A" (expected), [scratch] "=m" (scratch), [success] "=@ccz" (success)
            : [desired_lo] "Sm" ((uint32_t)desired), "c" ((uint32_t)(desired >> 32)), [dest] "D" (&storage)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#else // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "movl %%ebx, %[scratch]\n\t"
            "movl %[desired_lo], %%ebx\n\t"
            "lock; cmpxchg8b (%[dest])\n\t"
            "movl %[scratch], %%ebx\n\t"
            "sete %[success]\n\t"
#if !defined(BOOST_ATOMIC_DETAIL_NO_ASM_CONSTRAINT_ALTERNATIVES)
            : "+A,A,A,A,A,A" (expected), [scratch] "=m,m,m,m,m,m" (scratch), [success] "=q,m,q,m,q,m" (success)
            : [desired_lo] "S,S,D,D,m,m" ((uint32_t)desired), "c,c,c,c,c,c" ((uint32_t)(desired >> 32)), [dest] "D,D,S,S,D,D" (&storage)
#else
            : "+A" (expected), [scratch] "=m" (scratch), [success] "=q" (success)
            : [desired_lo] "S" ((uint32_t)desired), "c" ((uint32_t)(desired >> 32)), [dest] "D" (&storage)
#endif
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#endif // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)

        return success;

#else // defined(__PIC__)

        bool success;
#if defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchg8b %[dest]\n\t"
            : "+A" (expected), [dest] "+m" (storage), [success] "=@ccz" (success)
            : "b" ((uint32_t)desired), "c" ((uint32_t)(desired >> 32))
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#else // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchg8b %[dest]\n\t"
            "sete %[success]\n\t"
#if !defined(BOOST_ATOMIC_DETAIL_NO_ASM_CONSTRAINT_ALTERNATIVES)
            : "+A,A" (expected), [dest] "+m,m" (storage), [success] "=q,m" (success)
            : "b,b" ((uint32_t)desired), "c,c" ((uint32_t)(desired >> 32))
#else
            : "+A" (expected), [dest] "+m" (storage), [success] "=q" (success)
            : "b" ((uint32_t)desired), "c" ((uint32_t)(desired >> 32))
#endif
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#endif // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)

        return success;

#endif // defined(__PIC__)
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
#if defined(__clang__)
        // Clang cannot allocate eax:edx register pairs but it has sync intrinsics
        storage_type old_value = storage;
        while (true)
        {
            storage_type val = __sync_val_compare_and_swap(&storage, old_value, v);
            if (val == old_value)
                return val;
            old_value = val;
        }
#else // defined(__clang__)
#if defined(__PIC__)
        uint32_t scratch;
        __asm__ __volatile__
        (
            "movl %%ebx, %[scratch]\n\t"
            "movl %%eax, %%ebx\n\t"
            "movl %%edx, %%ecx\n\t"
            "movl %[dest_lo], %%eax\n\t"
            "movl %[dest_hi], %%edx\n\t"
            ".align 16\n\t"
            "1: lock; cmpxchg8b %[dest_lo]\n\t"
            "jne 1b\n\t"
            "movl %[scratch], %%ebx\n\t"
            : "+A" (v), [scratch] "=m" (scratch), [dest_lo] "+m" (storage), [dest_hi] "+m" (reinterpret_cast< volatile uint32_t* >(&storage)[1])
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "ecx", "memory"
        );
        return v;
#else // defined(__PIC__)
        storage_type old_value;
        __asm__ __volatile__
        (
            "movl %[dest_lo], %%eax\n\t"
            "movl %[dest_hi], %%edx\n\t"
            ".align 16\n\t"
            "1: lock; cmpxchg8b %[dest_lo]\n\t"
            "jne 1b\n\t"
            : "=&A" (old_value), [dest_lo] "+m" (storage), [dest_hi] "+m" (reinterpret_cast< volatile uint32_t* >(&storage)[1])
            : "b" ((uint32_t)v), "c" ((uint32_t)(v >> 32))
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        return old_value;
#endif // defined(__PIC__)
#endif // defined(__clang__)
    }
};

#endif // defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B)

#if defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)

template< bool Signed >
struct gcc_dcas_x86_64
{
    typedef typename make_storage_type< 16u, Signed >::type storage_type;
    typedef typename make_storage_type< 16u, Signed >::aligned aligned_storage_type;

    static BOOST_CONSTEXPR_OR_CONST bool is_always_lock_free = true;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        __asm__ __volatile__
        (
            "movq %[dest_lo], %%rax\n\t"
            "movq %[dest_hi], %%rdx\n\t"
            ".align 16\n\t"
            "1: lock; cmpxchg16b %[dest_lo]\n\t"
            "jne 1b\n\t"
            : [dest_lo] "=m" (storage), [dest_hi] "=m" (reinterpret_cast< volatile uint64_t* >(&storage)[1])
            : "b" (reinterpret_cast< const uint64_t* >(&v)[0]), "c" (reinterpret_cast< const uint64_t* >(&v)[1])
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "rax", "rdx", "memory"
        );
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order) BOOST_NOEXCEPT
    {
#if defined(__clang__)
        // Clang cannot allocate rax:rdx register pairs but it has sync intrinsics
        storage_type value = storage_type();
        return __sync_val_compare_and_swap(&storage, value, value);
#elif defined(BOOST_ATOMIC_DETAIL_NO_ASM_RAX_RDX_PAIRS)
        // GCC 4.4 can't allocate rax:rdx register pair either but it also doesn't support 128-bit __sync_val_compare_and_swap
        uint64_t value_bits[2];

        // We don't care for comparison result here; the previous value will be stored into value anyway.
        // Also we don't care for rbx and rcx values, they just have to be equal to rax and rdx before cmpxchg16b.
        __asm__ __volatile__
        (
            "movq %%rbx, %%rax\n\t"
            "movq %%rcx, %%rdx\n\t"
            "lock; cmpxchg16b %[storage]\n\t"
            : "=&a" (value_bits[0]), "=&d" (value_bits[1])
            : [storage] "m" (storage)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );

        storage_type value;
        BOOST_ATOMIC_DETAIL_MEMCPY(&value, value_bits, sizeof(value));
        return value;
#else // defined(BOOST_ATOMIC_DETAIL_NO_ASM_RAX_RDX_PAIRS)
        storage_type value;

        // We don't care for comparison result here; the previous value will be stored into value anyway.
        // Also we don't care for rbx and rcx values, they just have to be equal to rax and rdx before cmpxchg16b.
        __asm__ __volatile__
        (
            "movq %%rbx, %%rax\n\t"
            "movq %%rcx, %%rdx\n\t"
            "lock; cmpxchg16b %[storage]\n\t"
            : "=&A" (value)
            : [storage] "m" (storage)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );

        return value;
#endif
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
#if defined(__clang__)

        // Clang cannot allocate rax:rdx register pairs but it has sync intrinsics
        storage_type old_expected = expected;
        expected = __sync_val_compare_and_swap(&storage, old_expected, desired);
        return expected == old_expected;

#elif defined(BOOST_ATOMIC_DETAIL_NO_ASM_RAX_RDX_PAIRS)

        // GCC 4.4 can't allocate rax:rdx register pair either but it also doesn't support 128-bit __sync_val_compare_and_swap
        bool success;
        __asm__ __volatile__
        (
            "lock; cmpxchg16b %[dest]\n\t"
            "sete %[success]\n\t"
            : [dest] "+m" (storage), "+a" (reinterpret_cast< uint64_t* >(&expected)[0]), "+d" (reinterpret_cast< uint64_t* >(&expected)[1]), [success] "=q" (success)
            : "b" (reinterpret_cast< const uint64_t* >(&desired)[0]), "c" (reinterpret_cast< const uint64_t* >(&desired)[1])
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );

        return success;

#else // defined(BOOST_ATOMIC_DETAIL_NO_ASM_RAX_RDX_PAIRS)

        bool success;
#if defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchg16b %[dest]\n\t"
            : "+A" (expected), [dest] "+m" (storage), "=@ccz" (success)
            : "b" (reinterpret_cast< const uint64_t* >(&desired)[0]), "c" (reinterpret_cast< const uint64_t* >(&desired)[1])
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#else // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)
        __asm__ __volatile__
        (
            "lock; cmpxchg16b %[dest]\n\t"
            "sete %[success]\n\t"
#if !defined(BOOST_ATOMIC_DETAIL_NO_ASM_CONSTRAINT_ALTERNATIVES)
            : "+A,A" (expected), [dest] "+m,m" (storage), [success] "=q,m" (success)
            : "b,b" (reinterpret_cast< const uint64_t* >(&desired)[0]), "c,c" (reinterpret_cast< const uint64_t* >(&desired)[1])
#else
            : "+A" (expected), [dest] "+m" (storage), [success] "=q" (success)
            : "b" (reinterpret_cast< const uint64_t* >(&desired)[0]), "c" (reinterpret_cast< const uint64_t* >(&desired)[1])
#endif
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
#endif // defined(BOOST_ATOMIC_DETAIL_ASM_HAS_FLAG_OUTPUTS)

        return success;

#endif // defined(BOOST_ATOMIC_DETAIL_NO_ASM_RAX_RDX_PAIRS)
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
#if defined(__clang__)
        // Clang cannot allocate eax:edx register pairs but it has sync intrinsics
        storage_type old_val = storage;
        while (true)
        {
            storage_type val = __sync_val_compare_and_swap(&storage, old_val, v);
            if (val == old_val)
                return val;
            old_val = val;
        }
#elif defined(BOOST_ATOMIC_DETAIL_NO_ASM_RAX_RDX_PAIRS)
        // GCC 4.4 can't allocate rax:rdx register pair either but it also doesn't support 128-bit __sync_val_compare_and_swap
        uint64_t old_bits[2];
        __asm__ __volatile__
        (
            "movq %[dest_lo], %%rax\n\t"
            "movq %[dest_hi], %%rdx\n\t"
            ".align 16\n\t"
            "1: lock; cmpxchg16b %[dest_lo]\n\t"
            "jne 1b\n\t"
            : [dest_lo] "+m" (storage), [dest_hi] "+m" (reinterpret_cast< volatile uint64_t* >(&storage)[1]), "=&a" (old_bits[0]), "=&d" (old_bits[1])
            : "b" (reinterpret_cast< const uint64_t* >(&v)[0]), "c" (reinterpret_cast< const uint64_t* >(&v)[1])
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory", "rax", "rdx"
        );

        storage_type old_value;
        BOOST_ATOMIC_DETAIL_MEMCPY(&old_value, old_bits, sizeof(old_value));
        return old_value;
#else // defined(BOOST_ATOMIC_DETAIL_NO_ASM_RAX_RDX_PAIRS)
        storage_type old_value;
        __asm__ __volatile__
        (
            "movq %[dest_lo], %%rax\n\t"
            "movq %[dest_hi], %%rdx\n\t"
            ".align 16\n\t"
            "1: lock; cmpxchg16b %[dest_lo]\n\t"
            "jne 1b\n\t"
            : "=&A" (old_value), [dest_lo] "+m" (storage), [dest_hi] "+m" (reinterpret_cast< volatile uint64_t* >(&storage)[1])
            : "b" (reinterpret_cast< const uint64_t* >(&v)[0]), "c" (reinterpret_cast< const uint64_t* >(&v)[1])
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );

        return old_value;
#endif
    }
};

#endif // defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_X86_DCAS_HPP_INCLUDED_
