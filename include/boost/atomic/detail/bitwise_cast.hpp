/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2012 Tim Blechmann
 * Copyright (c) 2013-2018, 2020-2021 Andrey Semashev
 */
/*!
 * \file   atomic/detail/bitwise_cast.hpp
 *
 * This header defines \c bitwise_cast used to convert between storage and value types
 */

#ifndef BOOST_ATOMIC_DETAIL_BITWISE_CAST_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_BITWISE_CAST_HPP_INCLUDED_

#include <cstddef>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/addressof.hpp>
#include <boost/atomic/detail/string_ops.hpp>
#include <boost/atomic/detail/type_traits/integral_constant.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_GCC) && BOOST_GCC >= 80000
#pragma GCC diagnostic push
// copying an object of non-trivial type X from an array of Y. This is benign because we use memcpy to copy trivially copyable objects.
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

namespace boost {
namespace atomics {
namespace detail {

template< std::size_t ValueSize, typename To >
BOOST_FORCEINLINE void clear_tail_padding_bits(To& to, atomics::detail::true_type) BOOST_NOEXCEPT
{
    BOOST_ATOMIC_DETAIL_MEMSET(reinterpret_cast< unsigned char* >(atomics::detail::addressof(to)) + ValueSize, 0, sizeof(To) - ValueSize);
}

template< std::size_t ValueSize, typename To >
BOOST_FORCEINLINE void clear_tail_padding_bits(To&, atomics::detail::false_type) BOOST_NOEXCEPT
{
}

template< std::size_t ValueSize, typename To >
BOOST_FORCEINLINE void clear_tail_padding_bits(To& to) BOOST_NOEXCEPT
{
    atomics::detail::clear_tail_padding_bits< ValueSize >(to, atomics::detail::integral_constant< bool, ValueSize < sizeof(To) >());
}

template< typename To, std::size_t FromValueSize, typename From >
BOOST_FORCEINLINE To bitwise_cast(From const& from) BOOST_NOEXCEPT
{
    To to;
#if !defined(BOOST_ATOMIC_NO_CLEAR_PADDING)
    From from2(from);
    BOOST_ATOMIC_DETAIL_CLEAR_PADDING(atomics::detail::addressof(from2));
    BOOST_ATOMIC_DETAIL_MEMCPY
    (
        atomics::detail::addressof(to),
        atomics::detail::addressof(from2),
        (FromValueSize < sizeof(To) ? FromValueSize : sizeof(To))
    );
#else
    BOOST_ATOMIC_DETAIL_MEMCPY
    (
        atomics::detail::addressof(to),
        atomics::detail::addressof(from),
        (FromValueSize < sizeof(To) ? FromValueSize : sizeof(To))
    );
#endif
    atomics::detail::clear_tail_padding_bits< FromValueSize >(to);
    return to;
}

template< typename To, typename From >
BOOST_FORCEINLINE To bitwise_cast(From const& from) BOOST_NOEXCEPT
{
    return atomics::detail::bitwise_cast< To, sizeof(From) >(from);
}

} // namespace detail
} // namespace atomics
} // namespace boost

#if defined(BOOST_GCC) && BOOST_GCC >= 80000
#pragma GCC diagnostic pop
#endif

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_BITWISE_CAST_HPP_INCLUDED_
