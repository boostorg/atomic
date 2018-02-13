/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2012 Tim Blechmann
 * Copyright (c) 2013 - 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/bitwise_cast.hpp
 *
 * This header defines \c bitwise_cast used to convert between storage and value types
 */

#ifndef BOOST_ATOMIC_DETAIL_BITWISE_CAST_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_BITWISE_CAST_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/addressof.hpp>
#include <boost/atomic/detail/string_ops.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_GCC) && (BOOST_GCC+0) >= 40600
#pragma GCC diagnostic push
// missing initializer for member var
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename To, typename From >
BOOST_FORCEINLINE To bitwise_cast(From const& from) BOOST_NOEXCEPT
{
    struct
    {
        To to;
    }
    value = {};
    BOOST_ATOMIC_DETAIL_MEMCPY
    (
        atomics::detail::addressof(value.to),
        atomics::detail::addressof(from),
        (sizeof(From) < sizeof(To) ? sizeof(From) : sizeof(To))
    );
    return value.to;
}

} // namespace detail
} // namespace atomics
} // namespace boost

#if defined(BOOST_GCC) && (BOOST_GCC+0) >= 40600
#pragma GCC diagnostic pop
#endif

#endif // BOOST_ATOMIC_DETAIL_BITWISE_CAST_HPP_INCLUDED_
