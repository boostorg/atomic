/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2017 Dynatrace
 */
/*!
 * \file   atomic/detail/ops_xlcpp_zos.hpp
 *
 * This header contains an implementation of the \c operations template for the IBM z/OS XL C/C++ compiler.
 * It uses the compiler's intrinsics. The architecture is assumed to be z/Architecture.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_XLCPP_ZOS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_XLCPP_ZOS_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_type.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/capabilities.hpp>
#include <boost/atomic/detail/ops_cas_based.hpp>
#include <boost/atomic/detail/ops_extending_cas_based.hpp>
#include <boost/static_assert.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined(__IBMCPP__) || !defined(__COMPILER_VER__)
#error Compiler not supported.
#elif __COMPILER_VER__ < 0x41020000
// This implementation will probably also work for older compiler versions, but this should be tested before using it...
#error Compiler version not supported.
#elif !defined(__TOS_MVS__)
#error Target operating system not supported.
#endif

// IBM z/OS XL C/C++ V2.1 or higher

// __asm__ used only as a compiler barrier seems to work even without the ASM option/full ASM support.
#define BOOST_ATOMIC_DETAIL_XLCPP_ZOS_COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")

// The IBM z/OS XL C/C++ documentation explicitly warns about using '__cs1' or '__cds1' with option 'ANSIALIAS' and incompatible types - '__attribute__((__may_alias__))' should fix that.
// NOTE: With '__cs1' this shouldn't be a real problem, because the 'cs_t' used in the prototype is a typedef to 'unsigned int', which is what 'make_storage_type' gives us anyway.
//       With '__cds1' this is a problem though, since 'cds_t' is a union that doesn't even contain a 64 bit integer. And since Boost.Atomic requires 'storage_type' to be an integer type, we cannot use
//       'cds_t' as the storage type. So we're accessing an '(unsigned) long long' via a pointer to 'cds_t', which is a strict aliasing violation.
//       If the user compiles with option 'NOANSIALIAS', everything will be fine, but if he/she isn't, we'd better use '__attribute__((__may_alias__))'. And so we do.
#define BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS __attribute__((__may_alias__))

namespace boost {
namespace atomics {
namespace detail {

// Declare intrinsic CAS functions so we don't have to include <builtins.h> and/or <stdlib.h>.
// (The intrinsics work even if compiling without any of the compiler switches that would enable them -> no need to check the 'intrinsics supported' macro.)
extern "builtin" {
    int __cs1(void*, void*, void*);
    int __cds1(void*, void*, void*);
#if defined(BOOST_ATOMIC_DETAIL_XLCPP_ZOS_HAS_GRANDE_CAS)
    int __csg(void*, void*, void*);
    int __cdsg(void*, void*, void*);
#endif
} // extern "builtin"

struct xlcpp_zos_ops_base
{
    static BOOST_CONSTEXPR_OR_CONST bool is_always_lock_free = true;

    static BOOST_FORCEINLINE void hardware_full_fence() BOOST_NOEXCEPT
    {
        typedef make_storage_type<4u, false>::type BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS storage_type;
        typedef make_storage_type<4u, false>::aligned BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS aligned_storage_type;
        aligned_storage_type var;
        var.value = 0;
        storage_type expected = 0;
        storage_type desired = 0;
        __cs1(&expected, &var.value, &desired);
    }

    static BOOST_FORCEINLINE void fence_before(memory_order order) BOOST_NOEXCEPT
    {
        if ((order & memory_order_release) != 0)
            BOOST_ATOMIC_DETAIL_XLCPP_ZOS_COMPILER_BARRIER();
    }

    static BOOST_FORCEINLINE void fence_after(memory_order order) BOOST_NOEXCEPT
    {
        if ((order & memory_order_acquire) != 0)
            BOOST_ATOMIC_DETAIL_XLCPP_ZOS_COMPILER_BARRIER();
    }
};

#define BOOST_ATOMIC_DETAIL_XLCPP_ZOS_DEFINE_OPS_CAS(ClassName_, Size_, Cas_)                                          \
    template <bool Signed>                                                                                             \
    struct ClassName_ :                                                                                                \
        public xlcpp_zos_ops_base                                                                                      \
    {                                                                                                                  \
        /* Beware of aliasing - see definition of BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS */                           \
        typedef typename make_storage_type<Size_, Signed>::type BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS storage_type;  \
        typedef typename make_storage_type<Size_, Signed>::aligned BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS aligned_storage_type; \
                                                                                                                       \
       BOOST_STATIC_ASSERT(sizeof(storage_type) == Size_ && sizeof(aligned_storage_type) == Size_);                    \
                                                                                                                       \
       static BOOST_FORCEINLINE bool compare_exchange_strong(storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT \
       {                                                                                                               \
           fence_before(success_order);                                                                                \
           bool const success = Cas_(&expected, const_cast<storage_type*>(&storage), &desired) == 0;                   \
           if (success)                                                                                                \
               fence_after(success_order);                                                                             \
           else                                                                                                        \
               fence_after(failure_order);                                                                             \
           return success;                                                                                             \
       }                                                                                                               \
   };                                                                                                                  \
   // end BOOST_ATOMIC_DETAIL_XLCPP_ZOS_DEFINE_OPS_CAS 

BOOST_ATOMIC_DETAIL_XLCPP_ZOS_DEFINE_OPS_CAS(xlcpp_zos_ops_cas32,   4u, __cs1 )

#if defined(BOOST_ATOMIC_DETAIL_XLCPP_ZOS_HAS_GRANDE_CAS)
BOOST_ATOMIC_DETAIL_XLCPP_ZOS_DEFINE_OPS_CAS(xlcpp_zos_ops_cas64,   8u, __csg )
#else
BOOST_ATOMIC_DETAIL_XLCPP_ZOS_DEFINE_OPS_CAS(xlcpp_zos_ops_cas64,   8u, __cds1)
#endif

#if defined(BOOST_ATOMIC_DETAIL_XLCPP_ZOS_HAS_GRANDE_CAS) && (BOOST_ATOMIC_INT128_LOCK_FREE >= 2)
BOOST_ATOMIC_DETAIL_XLCPP_ZOS_DEFINE_OPS_CAS(xlcpp_zos_ops_cas128, 16u, __cdsg)
#endif

#undef BOOST_ATOMIC_DETAIL_XLCPP_ZOS_DEFINE_OPS_CAS

template <class CasImpl>
struct xlcpp_zos_ops :
    public CasImpl
{
    // Beware of aliasing - see definition of BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS
    typedef typename CasImpl::storage_type BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS storage_type;
    typedef typename CasImpl::aligned_storage_type BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS aligned_storage_type;

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type old_val;
        atomics::detail::non_atomic_load(storage, old_val);
        while (BOOST_UNLIKELY(!CasImpl::compare_exchange_strong(storage, old_val, v, order, memory_order_relaxed))) { }
        return old_val;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return CasImpl::compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }
};

template <class CasImpl>
struct xlcpp_zos_ops_simple_load_store :
    public xlcpp_zos_ops<CasImpl>
{
    // Beware of aliasing - see definition of BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS
    typedef typename CasImpl::storage_type BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS storage_type;
    typedef typename CasImpl::aligned_storage_type BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS aligned_storage_type;

    // Size can fit into one register and be loaded/stored with one single instruction -> can use simple load/store.
    BOOST_STATIC_ASSERT(sizeof(storage_type) <= sizeof(void*));
    BOOST_STATIC_ASSERT(sizeof(aligned_storage_type) <= sizeof(void*));

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_seq_cst)
        {
            fence_before(order);
            storage = v;
            fence_after(order);
        }
        else
            exchange(storage, v, memory_order_seq_cst);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        storage_type const v = storage;
        fence_after(order);
        return v;
    }
};

template <class CasImpl>
struct xlcpp_zos_ops_cas_load_store :
    public xlcpp_zos_ops<CasImpl>
{
    // Beware of aliasing - see definition of BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS
    typedef typename CasImpl::storage_type BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS storage_type;
    typedef typename CasImpl::aligned_storage_type BOOST_ATOMIC_DETAIL_XLCPP_ZOS_MAY_ALIAS aligned_storage_type;

    // Type is to big for a register -> have to use CAS to load/store.
    BOOST_STATIC_ASSERT(sizeof(storage_type) > sizeof(void*));
    BOOST_STATIC_ASSERT(sizeof(aligned_storage_type) > sizeof(void*));

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        exchange(storage, v, order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type value = storage_type();
        CasImpl::compare_exchange_strong(const_cast<storage_type volatile&>(storage), value, value, order, order);
        return value;
    }
};

template <bool Signed>
struct operations<4u, Signed> :
    public cas_based_operations<xlcpp_zos_ops_simple_load_store<xlcpp_zos_ops_cas32<Signed> > > { };

template <bool Signed>
struct operations<1u, Signed> :
    public extending_cas_based_operations<operations<4u, Signed>, 1u, Signed> { };

template <bool Signed>
struct operations<2u, Signed> :
    public extending_cas_based_operations<operations<4u, Signed>, 2u, Signed> { };

#if defined(__64BIT__)
template <bool Signed>
struct operations<8u, Signed> :
    public cas_based_operations<xlcpp_zos_ops_simple_load_store<xlcpp_zos_ops_cas64<Signed> > > { };
#else
template <bool Signed>
struct operations<8u, Signed> :
    public cas_based_operations<xlcpp_zos_ops_cas_load_store<xlcpp_zos_ops_cas64<Signed> > > { };
#endif

#if defined(BOOST_ATOMIC_DETAIL_XLCPP_ZOS_HAS_GRANDE_CAS) && (BOOST_ATOMIC_INT128_LOCK_FREE >= 2)
template <bool Signed>
struct operations<16u, Signed> :
    public cas_based_operations<xlcpp_zos_ops_cas_load_store<xlcpp_zos_ops_cas128<Signed> > > { };
#endif

BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    BOOST_ATOMIC_DETAIL_XLCPP_ZOS_COMPILER_BARRIER();

    if (order == memory_order_seq_cst)
    {
        xlcpp_zos_ops_base::hardware_full_fence();
        BOOST_ATOMIC_DETAIL_XLCPP_ZOS_COMPILER_BARRIER();
    }
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    if (order != memory_order_relaxed)
        BOOST_ATOMIC_DETAIL_XLCPP_ZOS_COMPILER_BARRIER();
}

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_XLCPP_ZOS_HPP_INCLUDED_
