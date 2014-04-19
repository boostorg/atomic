/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/atomic_flag.hpp
 *
 * This header contains interface definition of \c atomic_flag.
 */

#ifndef BOOST_ATOMIC_DETAIL_ATOMIC_FLAG_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_ATOMIC_FLAG_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {

#if defined(BOOST_NO_CXX11_CONSTEXPR)
#define BOOST_ATOMIC_NO_STATIC_INIT_ATOMIC_FLAG
#endif

#if !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX) && !defined(BOOST_ATOMIC_DEFAULT_INITIALIZE_ATOMIC_FLAG)
#define BOOST_ATOMIC_FLAG_INIT { 0 }
#else
namespace detail {
struct default_initializer {};
BOOST_CONSTEXPR_OR_CONST default_initializer default_init = {};
} // namespace detail
#define BOOST_ATOMIC_FLAG_INIT ::boost::atomics::detail::default_init
#endif

struct atomic_flag
{
    typedef atomics::detail::operations< 1u > operations;
    typedef operations::storage_type storage_type;

    storage_type m_storage;

#if !defined(BOOST_ATOMIC_DEFAULT_INITIALIZE_ATOMIC_FLAG)
#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
    BOOST_CONSTEXPR atomic_flag() BOOST_NOEXCEPT = default;
#else
    BOOST_CONSTEXPR atomic_flag() BOOST_NOEXCEPT {}
#endif
#if defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
    BOOST_CONSTEXPR atomic_flag(atomics::detail::default_initializer) BOOST_NOEXCEPT : m_storage(0) {}
#endif
#else
    BOOST_CONSTEXPR atomic_flag() BOOST_NOEXCEPT : m_storage(0) {}
    BOOST_CONSTEXPR atomic_flag(atomics::detail::default_initializer) BOOST_NOEXCEPT : m_storage(0) {}
#endif

    bool test_and_set(memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return operations::test_and_set(m_storage, order);
    }

    void clear(memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        operations::clear(m_storage, order);
    }

    BOOST_DELETED_FUNCTION(atomic_flag(atomic_flag const&))
    BOOST_DELETED_FUNCTION(atomic_flag& operator= (atomic_flag const&))
};

} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_ATOMIC_FLAG_HPP_INCLUDED_
