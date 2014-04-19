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
 * \file   atomic/detail/storage_types.hpp
 *
 * This header defines underlying types used as storage
 */

#ifndef BOOST_ATOMIC_DETAIL_STORAGE_TYPES_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_STORAGE_TYPES_HPP_INCLUDED_

#include <boost/cstdint.hpp>
#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

typedef boost::uint8_t storage8_t;
typedef boost::uint16_t storage16_t;
typedef boost::uint32_t storage32_t;

#if !defined(BOOST_NO_INT64_T)
typedef boost::uint64_t storage64_t;
#else
struct BOOST_ALIGNMENT(8) storage64_t
{
    boost::uint32_t data[2];
};

BOOST_FORCEINLINE bool operator== (storage64_t const& left, storage64_t const& right) BOOST_NOEXCEPT
{
    return left.data[0] == right.data[0] && left.data[1] == right.data[1];
}
BOOST_FORCEINLINE bool operator!= (storage64_t const& left, storage64_t const& right) BOOST_NOEXCEPT
{
    return !(left == right);
}
#endif

#if !defined(BOOST_HAS_INT128)
typedef boost::uint128_type storage128_t;
#else
struct BOOST_ALIGNMENT(16) storage128_t
{
    storage64_t data[2];
};

BOOST_FORCEINLINE bool operator== (storage128_t const& left, storage128_t const& right) BOOST_NOEXCEPT
{
    return left.data[0] == right.data[0] && left.data[1] == right.data[1];
}
BOOST_FORCEINLINE bool operator!= (storage128_t const& left, storage128_t const& right) BOOST_NOEXCEPT
{
    return !(left == right);
}
#endif

template< typename T >
struct storage_size_of
{
    enum _
    {
        size = sizeof(T),
        value = (size == 3 ? 4 : (size >= 5 && size <= 7 ? 8 : (size >= 9 && size <= 15 ? 16 : size)))
    };
};

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_STORAGE_TYPES_HPP_INCLUDED_
