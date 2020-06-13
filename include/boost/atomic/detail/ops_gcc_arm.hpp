/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_gcc_arm.hpp
 *
 * This header contains implementation of the \c operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_GCC_ARM_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_GCC_ARM_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/integral_conversions.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/detail/ops_gcc_arm_common.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

// From the ARM Architecture Reference Manual for architecture v6:
//
// LDREX{<cond>} <Rd>, [<Rn>]
// <Rd> Specifies the destination register for the memory word addressed by <Rd>
// <Rn> Specifies the register containing the address.
//
// STREX{<cond>} <Rd>, <Rm>, [<Rn>]
// <Rd> Specifies the destination register for the returned status value.
//      0  if the operation updates memory
//      1  if the operation fails to update memory
// <Rm> Specifies the register containing the word to be stored to memory.
// <Rn> Specifies the register containing the address.
// Rd must not be the same register as Rm or Rn.
//
// ARM v7 is like ARM v6 plus:
// There are half-word and byte versions of the LDREX and STREX instructions,
// LDREXH, LDREXB, STREXH and STREXB.
// There are also double-word versions, LDREXD and STREXD.
// (Actually it looks like these are available from version 6k onwards.)

template< bool Signed, bool Interprocess >
struct operations< 4u, Signed, Interprocess > :
    public gcc_arm_operations_base
{
    typedef typename storage_traits< 4u >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 4u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 4u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage = v;
        fence_after_store(order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v = storage;
        fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage_type original;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex %[original], %[storage]\n\t"          // load the original value
            "strex %[tmp], %[value], %[storage]\n\t"     // store the replacement, tmp = store failed
            "teq   %[tmp], #0\n\t"                       // check if store succeeded
            "bne   1b\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [tmp] "=&l" (tmp), [original] "=&r" (original), [storage] "+Q" (storage)
            : [value] "r" (v)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        uint32_t success;
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
        uint32_t tmp;
#endif
        storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "mov     %[success], #0\n\t"                      // success = 0
            "ldrex   %[original], %[storage]\n\t"             // original = *(&storage)
            "cmp     %[original], %[expected]\n\t"            // flags = original==expected
            "itt     eq\n\t"                                  // [hint that the following 2 instructions are conditional on flags.equal]
            "strexeq %[success], %[desired], %[storage]\n\t"  // if (flags.equal) *(&storage) = desired, success = store failed
            "eoreq   %[success], %[success], #1\n\t"          // if (flags.equal) success ^= 1 (i.e. make it 1 if store succeeded)
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),
              [success] "=&r" (success),
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
              [tmp] "=&l" (tmp),
#endif
              [storage] "+Q" (storage)
            : [expected] "Ir" (expected),
              [desired] "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = original;
        return !!success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        uint32_t success;
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
        uint32_t tmp;
#endif
        storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "mov     %[success], #0\n\t"                      // success = 0
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"             // original = *(&storage)
            "cmp     %[original], %[expected]\n\t"            // flags = original==expected
            "bne     2f\n\t"                                  // if (!flags.equal) goto end
            "strex   %[success], %[desired], %[storage]\n\t"  // *(&storage) = desired, success = store failed
            "eors    %[success], %[success], #1\n\t"          // success ^= 1 (i.e. make it 1 if store succeeded); flags.equal = success == 0
            "beq     1b\n\t"                                  // if (flags.equal) goto retry
            "2:\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),
              [success] "=&r" (success),
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
              [tmp] "=&l" (tmp),
#endif
              [storage] "+Q" (storage)
            : [expected] "Ir" (expected),
              [desired] "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = original;
        return !!success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n\t"  // result = original + value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n\t"  // result = original - value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "and     %[result], %[original], %[value]\n\t"  // result = original & value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "orr     %[result], %[original], %[value]\n\t"  // result = original | value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "eor     %[result], %[original], %[value]\n\t"  // result = original ^ value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }
};

#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXB_STREXB)

template< bool Signed, bool Interprocess >
struct operations< 1u, Signed, Interprocess > :
    public gcc_arm_operations_base
{
    typedef typename storage_traits< 1u >::type storage_type;
    typedef typename storage_traits< 4u >::type extended_storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 1u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 1u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage = v;
        fence_after_store(order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v = storage;
        fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        extended_storage_type original;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb %[original], %[storage]\n\t"          // load the original value and zero-extend to 32 bits
            "strexb %[tmp], %[value], %[storage]\n\t"     // store the replacement, tmp = store failed
            "teq    %[tmp], #0\n\t"                       // check if store succeeded
            "bne    1b\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [tmp] "=&l" (tmp), [original] "=&r" (original), [storage] "+Q" (storage)
            : [value] "r" (v)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        uint32_t success;
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
        uint32_t tmp;
#endif
        extended_storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "mov      %[success], #0\n\t"                      // success = 0
            "ldrexb   %[original], %[storage]\n\t"             // original = zero_extend(*(&storage))
            "cmp      %[original], %[expected]\n\t"            // flags = original==expected
            "itt      eq\n\t"                                  // [hint that the following 2 instructions are conditional on flags.equal]
            "strexbeq %[success], %[desired], %[storage]\n\t"  // if (flags.equal) *(&storage) = desired, success = store failed
            "eoreq    %[success], %[success], #1\n\t"          // if (flags.equal) success ^= 1 (i.e. make it 1 if store succeeded)
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),
              [success] "=&r" (success),
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
              [tmp] "=&l" (tmp),
#endif
              [storage] "+Q" (storage)
            : [expected] "Ir" (atomics::detail::zero_extend< extended_storage_type >(expected)),
              [desired] "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = static_cast< storage_type >(original);
        return !!success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        uint32_t success;
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
        uint32_t tmp;
#endif
        extended_storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "mov      %[success], #0\n\t"                      // success = 0
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"             // original = zero_extend(*(&storage))
            "cmp      %[original], %[expected]\n\t"            // flags = original==expected
            "bne      2f\n\t"                                  // if (!flags.equal) goto end
            "strexb   %[success], %[desired], %[storage]\n\t"  // *(&storage) = desired, success = store failed
            "eors     %[success], %[success], #1\n\t"          // success ^= 1 (i.e. make it 1 if store succeeded); flags.equal = success == 0
            "beq      1b\n\t"                                  // if (flags.equal) goto retry
            "2:\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),
              [success] "=&r" (success),
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
              [tmp] "=&l" (tmp),
#endif
              [storage] "+Q" (storage)
            : [expected] "Ir" (atomics::detail::zero_extend< extended_storage_type >(expected)),
              [desired] "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = static_cast< storage_type >(original);
        return !!success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "add      %[result], %[original], %[value]\n\t"  // result = original + value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "sub      %[result], %[original], %[value]\n\t"  // result = original - value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "and      %[result], %[original], %[value]\n\t"  // result = original & value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "orr      %[result], %[original], %[value]\n\t"  // result = original | value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "eor      %[result], %[original], %[value]\n\t"  // result = original ^ value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }
};

#else // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXB_STREXB)

template< bool Interprocess >
struct operations< 1u, false, Interprocess > :
    public operations< 4u, false, Interprocess >
{
    typedef operations< 4u, false, Interprocess > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n\t"  // result = original + value
            "uxtb    %[result], %[result]\n\t"              // zero extend result from 8 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        base_type::fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n\t"  // result = original - value
            "uxtb    %[result], %[result]\n\t"              // zero extend result from 8 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        base_type::fence_after(order);
        return original;
    }
};

template< bool Interprocess >
struct operations< 1u, true, Interprocess > :
    public operations< 4u, true, Interprocess >
{
    typedef operations< 4u, true, Interprocess > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n\t"  // result = original + value
            "sxtb    %[result], %[result]\n\t"              // sign extend result from 8 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        base_type::fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n\t"  // result = original - value
            "sxtb    %[result], %[result]\n\t"              // sign extend result from 8 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        base_type::fence_after(order);
        return original;
    }
};

#endif // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXB_STREXB)

#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXH_STREXH)

template< bool Signed, bool Interprocess >
struct operations< 2u, Signed, Interprocess > :
    public gcc_arm_operations_base
{
    typedef typename storage_traits< 2u >::type storage_type;
    typedef typename storage_traits< 4u >::type extended_storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 2u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 2u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage = v;
        fence_after_store(order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v = storage;
        fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        extended_storage_type original;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh %[original], %[storage]\n\t"          // load the original value and zero-extend to 32 bits
            "strexh %[tmp], %[value], %[storage]\n\t"     // store the replacement, tmp = store failed
            "teq    %[tmp], #0\n\t"                       // check if store succeeded
            "bne    1b\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [tmp] "=&l" (tmp), [original] "=&r" (original), [storage] "+Q" (storage)
            : [value] "r" (v)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        uint32_t success;
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
        uint32_t tmp;
#endif
        extended_storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "mov      %[success], #0\n\t"                      // success = 0
            "ldrexh   %[original], %[storage]\n\t"             // original = zero_extend(*(&storage))
            "cmp      %[original], %[expected]\n\t"            // flags = original==expected
            "itt      eq\n\t"                                  // [hint that the following 2 instructions are conditional on flags.equal]
            "strexheq %[success], %[desired], %[storage]\n\t"  // if (flags.equal) *(&storage) = desired, success = store failed
            "eoreq    %[success], %[success], #1\n\t"          // if (flags.equal) success ^= 1 (i.e. make it 1 if store succeeded)
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),
              [success] "=&r" (success),
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
              [tmp] "=&l" (tmp),
#endif
              [storage] "+Q" (storage)
            : [expected] "Ir" (atomics::detail::zero_extend< extended_storage_type >(expected)),
              [desired] "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = static_cast< storage_type >(original);
        return !!success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        uint32_t success;
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
        uint32_t tmp;
#endif
        extended_storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "mov      %[success], #0\n\t"                      // success = 0
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"             // original = zero_extend(*(&storage))
            "cmp      %[original], %[expected]\n\t"            // flags = original==expected
            "bne      2f\n\t"                                  // if (!flags.equal) goto end
            "strexh   %[success], %[desired], %[storage]\n\t"  // *(&storage) = desired, success = store failed
            "eors     %[success], %[success], #1\n\t"          // success ^= 1 (i.e. make it 1 if store succeeded); flags.equal = success == 0
            "beq      1b\n\t"                                  // if (flags.equal) goto retry
            "2:\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),
              [success] "=&r" (success),
#if !defined(BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED)
              [tmp] "=&l" (tmp),
#endif
              [storage] "+Q" (storage)
            : [expected] "Ir" (atomics::detail::zero_extend< extended_storage_type >(expected)),
              [desired] "r" (desired)
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = static_cast< storage_type >(original);
        return !!success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "add      %[result], %[original], %[value]\n\t"  // result = original + value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "sub      %[result], %[original], %[value]\n\t"  // result = original - value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "and      %[result], %[original], %[value]\n\t"  // result = original & value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "orr      %[result], %[original], %[value]\n\t"  // result = original | value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "eor      %[result], %[original], %[value]\n\t"  // result = original ^ value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }
};

#else // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXH_STREXH)

template< bool Interprocess >
struct operations< 2u, false, Interprocess > :
    public operations< 4u, false, Interprocess >
{
    typedef operations< 4u, false, Interprocess > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n\t"  // result = original + value
            "uxth    %[result], %[result]\n\t"              // zero extend result from 16 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        base_type::fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n\t"  // result = original - value
            "uxth    %[result], %[result]\n\t"              // zero extend result from 16 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        base_type::fence_after(order);
        return original;
    }
};

template< bool Interprocess >
struct operations< 2u, true, Interprocess > :
    public operations< 4u, true, Interprocess >
{
    typedef operations< 4u, true, Interprocess > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n\t"  // result = original + value
            "sxth    %[result], %[result]\n\t"              // sign extend result from 16 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        base_type::fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n\t"  // result = original - value
            "sxth    %[result], %[result]\n\t"              // sign extend result from 16 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        base_type::fence_after(order);
        return original;
    }
};

#endif // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXH_STREXH)

#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXD_STREXD)

// Unlike 32-bit operations, for 64-bit loads and stores we must use ldrexd/strexd.
// Any other instructions result in a non-atomic sequence of 32-bit accesses.
// See "ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition",
// Section A3.5.3 "Atomicity in the ARM architecture".

// In the asm blocks below we have to use 32-bit register pairs to compose 64-bit values.
// In order to pass the 64-bit operands to/from asm blocks, we use undocumented gcc feature:
// the lower half (Rt) of the operand is accessible normally, via the numbered placeholder (e.g. %0),
// and the upper half (Rt2) - via the same placeholder with an 'H' after the '%' sign (e.g. %H0).
// See: http://hardwarebug.org/2010/07/06/arm-inline-asm-secrets/

template< bool Signed, bool Interprocess >
struct operations< 8u, Signed, Interprocess > :
    public gcc_arm_operations_base
{
    typedef typename storage_traits< 8u >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 8u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 8u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        exchange(storage, v, order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        // To implement the load we still have to use ldrexd+strexd pair so that the store resets
        // the exclusive access mark on the storage address that is set by the load. The technique
        // is described in ARM Architecture Reference Manual ARMv8, Section B2.2.1.
        storage_type original;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd %1, %H1, [%2]\n\t"        // load the value
            "strexd %0, %1, %H1, [%2]\n\t"    // store the loaded value back, tmp = store failed
            "teq    %0, #0\n\t"               // check if store succeeded
            "bne    1b\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original)   // %1
            : "r" (&storage)     // %2
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage_type original;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd %1, %H1, [%3]\n\t"        // load the original value
            "strexd %0, %2, %H2, [%3]\n\t"    // store the replacement, tmp = store failed
            "teq    %0, #0\n\t"               // check if store succeeded
            "bne    1b\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original)   // %1
            : "r" (v),           // %2
              "r" (&storage)     // %3
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        storage_type original, old_val = expected;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "ldrexd   %1, %H1, [%3]\n\t"               // original = *(&storage)
            "cmp      %1, %2\n\t"                      // flags = original.lo==old_val.lo
            "ittt     eq\n\t"                          // [hint that the following 3 instructions are conditional on flags.equal]
            "cmpeq    %H1, %H2\n\t"                    // if (flags.equal) flags = original.hi==old_val.hi
            "strexdeq %0, %4, %H4, [%3]\n\t"           // if (flags.equal) *(&storage) = desired, tmp = store failed
            "teqeq    %0, #0\n\t"                      // if (flags.equal) flags = tmp==0
            "ite      eq\n\t"                          // [hint that the following 2 instructions are conditional on flags.equal]
            "moveq    %2, #1\n\t"                      // if (flags.equal) old_val.lo = 1
            "movne    %2, #0\n\t"                      // if (!flags.equal) old_val.lo = 0
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "+r" (old_val)     // %2
            : "r" (&storage),    // %3
              "r" (desired)      // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        const uint32_t success = (uint32_t)old_val;
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = original;
        return !!success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        storage_type original, old_val = expected;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, [%3]\n\t"               // original = *(&storage)
            "cmp     %1, %2\n\t"                      // flags = original.lo==old_val.lo
            "it      eq\n\t"                          // [hint that the following instruction is conditional on flags.equal]
            "cmpeq   %H1, %H2\n\t"                    // if (flags.equal) flags = original.hi==old_val.hi
            "bne     2f\n\t"                          // if (!flags.equal) goto end
            "strexd  %0, %4, %H4, [%3]\n\t"           // *(&storage) = desired, tmp = store failed
            "teq     %0, #0\n\t"                      // flags.equal = tmp == 0
            "bne     1b\n\t"                          // if (flags.equal) goto retry
            "2:\n\t"
            "ite      eq\n\t"                         // [hint that the following 2 instructions are conditional on flags.equal]
            "moveq    %2, #1\n\t"                     // if (flags.equal) old_val.lo = 1
            "movne    %2, #0\n\t"                     // if (!flags.equal) old_val.lo = 0
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "+r" (old_val)     // %2
            : "r" (&storage),    // %3
              "r" (desired)      // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        const uint32_t success = (uint32_t)old_val;
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = original;
        return !!success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, [%3]\n\t"               // original = *(&storage)
            "adds    %2, %1, %4\n\t"                  // result = original + value
            "adc     %H2, %H1, %H4\n\t"
            "strexd  %0, %2, %H2, [%3]\n\t"           // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result)     // %2
            : "r" (&storage),    // %3
              "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, [%3]\n\t"               // original = *(&storage)
            "subs    %2, %1, %4\n\t"                  // result = original - value
            "sbc     %H2, %H1, %H4\n\t"
            "strexd  %0, %2, %H2, [%3]\n\t"           // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result)     // %2
            : "r" (&storage),    // %3
              "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, [%3]\n\t"               // original = *(&storage)
            "and     %2, %1, %4\n\t"                  // result = original & value
            "and     %H2, %H1, %H4\n\t"
            "strexd  %0, %2, %H2, [%3]\n\t"           // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result)     // %2
            : "r" (&storage),    // %3
              "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, [%3]\n\t"               // original = *(&storage)
            "orr     %2, %1, %4\n\t"                  // result = original | value
            "orr     %H2, %H1, %H4\n\t"
            "strexd  %0, %2, %H2, [%3]\n\t"           // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result)     // %2
            : "r" (&storage),    // %3
              "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, [%3]\n\t"               // original = *(&storage)
            "eor     %2, %1, %4\n\t"                  // result = original ^ value
            "eor     %H2, %H1, %H4\n\t"
            "strexd  %0, %2, %H2, [%3]\n\t"           // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result)     // %2
            : "r" (&storage),    // %3
              "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }
};

#endif // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXD_STREXD)


BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    if (order != memory_order_relaxed)
        gcc_arm_operations_base::hardware_full_fence();
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    if (order != memory_order_relaxed)
        __asm__ __volatile__ ("" ::: "memory");
}

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_ARM_HPP_INCLUDED_
