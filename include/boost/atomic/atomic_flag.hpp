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
 * \file   atomic/atomic_flag.hpp
 *
 * This header contains definition of \c atomic_flag.
 */

#ifndef BOOST_ATOMIC_ATOMIC_FLAG_HPP_INCLUDED_
#define BOOST_ATOMIC_ATOMIC_FLAG_HPP_INCLUDED_

#include <boost/assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/capabilities.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/operations.hpp>
#include <boost/atomic/detail/wait_operations.hpp>
#include <boost/atomic/detail/aligned_variable.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

/*
 * IMPLEMENTATION NOTE: All interface functions MUST be declared with BOOST_FORCEINLINE,
 *                      see comment for convert_memory_order_to_gcc in ops_gcc_atomic.hpp.
 */

namespace boost {
namespace atomics {

#if defined(BOOST_ATOMIC_DETAIL_NO_CXX11_CONSTEXPR_UNION_INIT) || defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
#define BOOST_ATOMIC_NO_ATOMIC_FLAG_INIT
#else
#define BOOST_ATOMIC_FLAG_INIT {}
#endif

//! Atomic flag
struct atomic_flag
{
    // Prefer 4-byte storage as most platforms support waiting/notifying operations without a lock pool for 32-bit integers
    typedef atomics::detail::operations< 4u, false > operations;
    typedef atomics::detail::wait_operations< operations > wait_operations;
    typedef operations::storage_type storage_type;

    BOOST_ATOMIC_DETAIL_ALIGNED_VAR(operations::storage_alignment, storage_type, m_storage);

    BOOST_FORCEINLINE BOOST_ATOMIC_DETAIL_CONSTEXPR_UNION_INIT atomic_flag() BOOST_NOEXCEPT : m_storage(0u)
    {
    }

    BOOST_FORCEINLINE bool test(memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        BOOST_ASSERT(order != memory_order_release);
        BOOST_ASSERT(order != memory_order_acq_rel);
        return !!operations::load(m_storage, order);
    }

    BOOST_FORCEINLINE bool test_and_set(memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return operations::test_and_set(m_storage, order);
    }

    BOOST_FORCEINLINE void clear(memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        BOOST_ASSERT(order != memory_order_consume);
        BOOST_ASSERT(order != memory_order_acquire);
        BOOST_ASSERT(order != memory_order_acq_rel);
        operations::clear(m_storage, order);
    }

    BOOST_FORCEINLINE bool wait(bool old_val, memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        BOOST_ASSERT(order != memory_order_release);
        BOOST_ASSERT(order != memory_order_acq_rel);

        return !!wait_operations::wait(m_storage, static_cast< storage_type >(old_val), order);
    }

    BOOST_FORCEINLINE void notify_one() volatile BOOST_NOEXCEPT
    {
        wait_operations::notify_one(m_storage);
    }

    BOOST_FORCEINLINE void notify_all() volatile BOOST_NOEXCEPT
    {
        wait_operations::notify_all(m_storage);
    }

    BOOST_DELETED_FUNCTION(atomic_flag(atomic_flag const&))
    BOOST_DELETED_FUNCTION(atomic_flag& operator= (atomic_flag const&))
};

} // namespace atomics

using atomics::atomic_flag;

} // namespace boost

#endif // BOOST_ATOMIC_ATOMIC_FLAG_HPP_INCLUDED_
