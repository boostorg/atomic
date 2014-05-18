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

#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_type.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/detail/ops_extending_cas_based.hpp>
#include <boost/atomic/capabilities.hpp>

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
// FIXME these are not yet used; should be mostly a matter of copy-and-paste.
// I think you can supply an immediate offset to the address.
//
// A memory barrier is effected using a "co-processor 15" instruction,
// though a separate assembler mnemonic is available for it in v7.
//
// "Thumb 1" is a subset of the ARM instruction set that uses a 16-bit encoding.  It
// doesn't include all instructions and in particular it doesn't include the co-processor
// instruction used for the memory barrier or the load-locked/store-conditional
// instructions.  So, if we're compiling in "Thumb 1" mode, we need to wrap all of our
// asm blocks with code to temporarily change to ARM mode.
//
// You can only change between ARM and Thumb modes when branching using the bx instruction.
// bx takes an address specified in a register.  The least significant bit of the address
// indicates the mode, so 1 is added to indicate that the destination code is Thumb.
// A temporary register is needed for the address and is passed as an argument to these
// macros.  It must be one of the "low" registers accessible to Thumb code, specified
// using the "l" attribute in the asm statement.
//
// Architecture v7 introduces "Thumb 2", which does include (almost?) all of the ARM
// instruction set.  (Actually, there was an extension of v6 called v6T2 which supported
// "Thumb 2" mode, but its architecture manual is no longer available, referring to v7.)
// So in v7 we don't need to change to ARM mode; we can write "universal
// assembler" which will assemble to Thumb 2 or ARM code as appropriate.  The only thing
// we need to do to make this "universal" assembler mode work is to insert "IT" instructions
// to annotate the conditional instructions.  These are ignored in other modes (e.g. v6),
// so they can always be present.

#if defined(__thumb__) && !defined(__thumb2__)
#define BOOST_ATOMIC_DETAIL_ARM_ASM_START(TMPREG) "adr " #TMPREG ", 8f\n" "bx " #TMPREG "\n" ".arm\n" ".align 4\n" "8: "
#define BOOST_ATOMIC_DETAIL_ARM_ASM_END(TMPREG)   "adr " #TMPREG ", 9f + 1\n" "bx " #TMPREG "\n" ".thumb\n" ".align 2\n" "9: "
#else
// The tmpreg may be wasted in this case, which is non-optimal.
#define BOOST_ATOMIC_DETAIL_ARM_ASM_START(TMPREG)
#define BOOST_ATOMIC_DETAIL_ARM_ASM_END(TMPREG)
#endif

struct gcc_arm_operations_base
{
    static BOOST_FORCEINLINE void fence_before(memory_order order) BOOST_NOEXCEPT
    {
        if ((order & memory_order_release) != 0)
            hardware_full_fence();
    }

    static BOOST_FORCEINLINE void fence_after(memory_order order) BOOST_NOEXCEPT
    {
        if ((order & memory_order_acquire) != 0)
            hardware_full_fence();
    }

    static BOOST_FORCEINLINE void fence_after_store(memory_order order) BOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
            hardware_full_fence();
    }

    static BOOST_FORCEINLINE void hardware_full_fence() BOOST_NOEXCEPT
    {
#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_DMB)
        // Older binutils (supposedly, older than 2.21.1) didn't support symbolic arguments of the "dmb" instruction such as "ish".
        // So we use its equivalent instead: a numeric constant 11. Another solution would be to inject encoded bytes of the instruction:
        // ".byte 0xbf, 0xf3, 0x5b, 0x8f\n"
        __asm__ __volatile__
        (
            "dmb #11\n" // dmb ish
            :
            :
            : "memory"
        );
#else
        int tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "mcr\tp15, 0, r0, c7, c10, 5\n"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : "=&l" (tmp)
            :
            : "memory"
        );
#endif
    }
};


template< bool Signed >
struct operations< 4u, Signed > :
    public gcc_arm_operations_base
{
    typedef typename make_storage_type< 4u, Signed >::type storage_type;

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
        storage_type original;
        fence_before(order);
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n"
            "ldrex %[original], %[storage]\n"          // load the original value
            "strex %[tmp], %[value], %[storage]\n"     // store the replacement, tmp = store failed
            "teq   %[tmp], #0\n"                       // check if store succeeded
            "bne   1b\n"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [tmp] "=&l" (tmp), [original] "=&r" (original), [storage] "+Q" (storage)
            : [value] "r" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        uint32_t success;
        uint32_t tmp;
        storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "mov     %[success], #0\n"                      // success = 0
            "ldrex   %[original], %[storage]\n"             // original = *(&storage)
            "teq     %[original], %[expected]\n"            // flags = original==expected
            "itt     eq\n"                                  // [hint that the following 2 instructions are conditional on flags.equal]
            "strexeq %[success], %[desired], %[storage]\n"  // if (flags.equal) *(&storage) = desired, success = store failed
            "eoreq   %[success], %[success], #1\n"          // if (flags.equal) success ^= 1 (i.e. make it 1 if store succeeded)
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [success] "=&r" (success),    // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [expected] "r" (expected),    // %4
              [desired] "r" (desired)       // %5
            : "cc"
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
        uint32_t tmp;
        storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "mov     %[success], #0\n"                      // success = 0
            "1:\n"
            "ldrex   %[original], %[storage]\n"             // original = *(&storage)
            "teq     %[original], %[expected]\n"            // flags = original==expected
            "bne     2f\n"                                  // if (!flags.equal) goto end
            "strex   %[success], %[desired], %[storage]\n"  // *(&storage) = desired, success = store failed
            "eors    %[success], %[success], #1\n"          // success ^= 1 (i.e. make it 1 if store succeeded); flags.equal = success == 0
            "beq     1b\n"                                  // if (flags.equal) goto retry
            "2:\n"
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [success] "=&r" (success),    // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [expected] "r" (expected),    // %4
              [desired] "r" (desired)       // %5
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n"  // result = original + value
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n"  // result = original - value
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "and     %[result], %[original], %[value]\n"  // result = original & value
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "orr     %[result], %[original], %[value]\n"  // result = original | value
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "eor     %[result], %[original], %[value]\n"  // result = original ^ value
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
        store(storage, 0, order);
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }
};


template< >
struct operations< 1u, false > :
    public operations< 4u, false >
{
    typedef operations< 4u, false > base_type;
    typedef base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n"  // result = original + value
            "uxtb    %[result], %[result]\n"              // zero extend result from 8 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n"  // result = original - value
            "uxtb    %[result], %[result]\n"              // zero extend result from 8 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
        );
        fence_after(order);
        return original;
    }
};

template< >
struct operations< 1u, true > :
    public operations< 4u, true >
{
    typedef operations< 4u, true > base_type;
    typedef base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n"  // result = original + value
            "sxtb    %[result], %[result]\n"              // sign extend result from 8 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n"  // result = original - value
            "sxtb    %[result], %[result]\n"              // sign extend result from 8 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
        );
        fence_after(order);
        return original;
    }
};


template< >
struct operations< 2u, false > :
    public operations< 4u, false >
{
    typedef operations< 4u, false > base_type;
    typedef base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n"  // result = original + value
            "uxth    %[result], %[result]\n"              // zero extend result from 16 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n"  // result = original - value
            "uxth    %[result], %[result]\n"              // zero extend result from 16 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
        );
        fence_after(order);
        return original;
    }
};

template< >
struct operations< 2u, true > :
    public operations< 4u, true >
{
    typedef operations< 4u, true > base_type;
    typedef base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n"  // result = original + value
            "sxth    %[result], %[result]\n"              // sign extend result from 16 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
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
            "1:\n"
            "ldrex   %[original], %[storage]\n"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n"  // result = original - value
            "sxth    %[result], %[result]\n"              // sign extend result from 16 to 32 bits
            "strex   %[tmp], %[result], %[storage]\n"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n"                        // flags = tmp==0
            "bne     1b\n"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "r" (v)               // %4
            : "cc"
        );
        fence_after(order);
        return original;
    }
};


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

template< bool Signed >
struct operations< 8u, Signed > :
    public gcc_arm_operations_base
{
    typedef typename make_storage_type< 8u, Signed >::type storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        exchange(storage, v, order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "ldrexd %1, %H1, %2\n"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : "=&l" (tmp),       // %0
              "=&r" (original)   // %1
            : "Q" (storage)      // %2
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        fence_before(order);
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n"
            "ldrexd %1, %H1, %2\n"          // load the original value
            "strexd %0, %3, %H3, %2\n"      // store the replacement, tmp = store failed
            "teq    %0, #0\n"               // check if store succeeded
            "bne    1b\n"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : "=&l" (tmp),       // %0
              "=&r" (original),  // %1
              "+Q" (storage)     // %2
            : "r" (v)            // %3
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        fence_before(success_order);
        uint32_t success;
        uint32_t tmp;
        storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            "ldrexd  %0, %H0, %3\n"                 // original = *(&storage)
            "eor     %1, %0, %4\n"                  // The three instructions are just a fancy way of comparing 2 64-bit integers:
            "eor     %2, %H0, %H4\n"                // success = original[lo] ^ expected[lo]; tmp = original[hi] ^ expected[hi];
            "orrs    %1, %1, %2\n"                  // success = success | tmp (i.e. 0 if original==expected); flags = original==expected
            "itte    eq\n"                          // [hint that the following 3 instructions are conditional on flags.equal]
            "strexdeq %1, %5, %H5, %3\n"            // if (flags.equal) *(&storage) = desired, success = store failed
            "eoreq   %1, %1, #1\n"                  // if (flags.equal) success ^= 1 (i.e. make it 1 if store succeeded)
            "movne   %1, #0\n"                      // if (!flags.equal) success = 0
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            : "=&r" (original),  // %0
              "=&r" (success),   // %1
              "=&l" (tmp),       // %2
              "+Q" (storage)     // %3
            : "r" (expected),    // %4
              "r" (desired)      // %5
            : "cc"
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
        uint32_t tmp;
        storage_type original;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            "1:\n"
            "ldrexd  %0, %H0, %3\n"                 // original = *(&storage)
            "eor     %1, %0, %4\n"                  // The three instructions are just a fancy way of comparing 2 64-bit integers:
            "eor     %2, %H0, %H4\n"                // success = original[lo] ^ expected[lo]; tmp = original[hi] ^ expected[hi];
            "orrs    %1, %1, %2\n"                  // success = success | tmp (i.e. 0 if original==expected); flags = original==expected
            "itt     ne\n"                          // [hint that the following 2 instructions are conditional on flags.equal]
            "movne   %1, #0\n"                      // if (!flags.equal) success = 0
            "bne     2f\n"                          // if (!flags.equal) goto end
            "strexd  %1, %5, %H5, %3\n"             // *(&storage) = desired, success = store failed
            "eors    %1, %1, #1\n"                  // success ^= 1 (i.e. make it 1 if store succeeded); flags.equal = success == 0
            "beq     1b\n"                          // if (flags.equal) goto retry
            "2:\n"
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            : "=&r" (original),  // %0
              "=&r" (success),   // %1
              "=&l" (tmp),       // %2
              "+Q" (storage)     // %3
            : "r" (expected),    // %4
              "r" (desired)      // %5
            : "cc"
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
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            "1:\n"
            "ldrexd  %0, %H0, %3\n"                 // original = *(&storage)
            "adds    %1, %0, %4\n"                  // result = original + value
            "adc     %H1, %H0, %H4\n"
            "strexd  %2, %1, %H1, %3\n"             // *(&storage) = result, tmp = store failed
            "teq     %2, #0\n"                      // flags = tmp==0
            "bne     1b\n"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            : "=&r" (original),  // %0
              "=&r" (result),    // %1
              "=&l" (tmp),       // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : "cc"
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
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            "1:\n"
            "ldrexd  %0, %H0, %3\n"                 // original = *(&storage)
            "subs    %1, %0, %4\n"                  // result = original - value
            "sbc     %H1, %H0, %H4\n"
            "strexd  %2, %1, %H1, %3\n"             // *(&storage) = result, tmp = store failed
            "teq     %2, #0\n"                      // flags = tmp==0
            "bne     1b\n"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            : "=&r" (original),  // %0
              "=&r" (result),    // %1
              "=&l" (tmp),       // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : "cc"
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
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            "1:\n"
            "ldrexd  %0, %H0, %3\n"                 // original = *(&storage)
            "and     %1, %0, %4\n"                  // result = original & value
            "and     %H1, %H0, %H4\n"
            "strexd  %2, %1, %H1, %3\n"             // *(&storage) = result, tmp = store failed
            "teq     %2, #0\n"                      // flags = tmp==0
            "bne     1b\n"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            : "=&r" (original),  // %0
              "=&r" (result),    // %1
              "=&l" (tmp),       // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : "cc"
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
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            "1:\n"
            "ldrexd  %0, %H0, %3\n"                 // original = *(&storage)
            "orr     %1, %0, %4\n"                  // result = original | value
            "orr     %H1, %H0, %H4\n"
            "strexd  %2, %1, %H1, %3\n"             // *(&storage) = result, tmp = store failed
            "teq     %2, #0\n"                      // flags = tmp==0
            "bne     1b\n"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            : "=&r" (original),  // %0
              "=&r" (result),    // %1
              "=&l" (tmp),       // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : "cc"
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
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            "1:\n"
            "ldrexd  %0, %H0, %3\n"                 // original = *(&storage)
            "eor     %1, %0, %4\n"                  // result = original ^ value
            "eor     %H1, %H0, %H4\n"
            "strexd  %2, %1, %H1, %3\n"             // *(&storage) = result, tmp = store failed
            "teq     %2, #0\n"                      // flags = tmp==0
            "bne     1b\n"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%2)
            : "=&r" (original),  // %0
              "=&r" (result),    // %1
              "=&l" (tmp),       // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : "cc"
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
        store(storage, 0, order);
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }
};

#endif // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXD_STREXD)


BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    if ((order & (memory_order_acquire | memory_order_release)) != 0)
        gcc_arm_operations_base::hardware_full_fence();
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    if ((order & ~memory_order_consume) != 0)
        __asm__ __volatile__ ("" ::: "memory");
}

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_ARM_HPP_INCLUDED_
