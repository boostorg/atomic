/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2015 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ext_ops_generic.hpp
 *
 * This header contains generic implementation of the extended atomic operations.
 */

#ifndef BOOST_ATOMIC_DETAIL_EXT_OPS_GENERIC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_EXT_OPS_GENERIC_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_type.hpp>
#include <boost/atomic/capabilities.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename Base, std::size_t Size, bool Signed >
struct generic_extended_operations :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;
    typedef typename make_storage_type< Size, Signed >::type emulated_storage_type;
    typedef typename make_storage_type< Size, false >::type unsigned_emulated_storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type old_val;
        atomics::detail::non_atomic_load(storage, old_val);
        while (!Base::compare_exchange_weak(storage, old_val, static_cast< storage_type >(-static_cast< emulated_storage_type >(old_val)), order, memory_order_relaxed)) {}
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return Base::fetch_xor(storage, static_cast< storage_type >(~static_cast< emulated_storage_type >(0)), order);
    }

    static BOOST_FORCEINLINE void opaque_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        Base::fetch_add(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        Base::fetch_sub(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        fetch_negate(storage, order);
    }

    static BOOST_FORCEINLINE void opaque_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        Base::fetch_and(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        Base::fetch_or(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        Base::fetch_xor(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        fetch_complement(storage, order);
    }

    static BOOST_FORCEINLINE bool add_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type old_val = Base::fetch_add(storage, v, order);
        emulated_storage_type new_val = static_cast< emulated_storage_type >(old_val) + static_cast< emulated_storage_type >(v);
        return !new_val;
    }

    static BOOST_FORCEINLINE bool sub_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type old_val = Base::fetch_sub(storage, v, order);
        emulated_storage_type new_val = static_cast< emulated_storage_type >(old_val) - static_cast< emulated_storage_type >(v);
        return !new_val;
    }

    static BOOST_FORCEINLINE bool and_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return !(Base::fetch_and(storage, v, order) & v);
    }

    static BOOST_FORCEINLINE bool or_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return !(Base::fetch_or(storage, v, order) | v);
    }

    static BOOST_FORCEINLINE bool xor_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return !(Base::fetch_xor(storage, v, order) ^ v);
    }

    static BOOST_FORCEINLINE bool bit_test_and_set(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        storage_type mask = storage_type(((unsigned_emulated_storage_type)1u) << bit_number);
        storage_type old_val = Base::fetch_or(storage, mask, order);
        return !!(old_val & mask);
    }

    static BOOST_FORCEINLINE bool bit_test_and_reset(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        storage_type mask = storage_type(((unsigned_emulated_storage_type)1u) << bit_number);
        storage_type old_val = Base::fetch_and(storage, ~mask, order);
        return !!(old_val & mask);
    }

    static BOOST_FORCEINLINE bool bit_test_and_complement(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        storage_type mask = storage_type(((unsigned_emulated_storage_type)1u) << bit_number);
        storage_type old_val = Base::fetch_xor(storage, mask, order);
        return !!(old_val & mask);
    }
};

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_EXT_OPS_GENERIC_HPP_INCLUDED_
