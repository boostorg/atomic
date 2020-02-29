/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2012 Tim Blechmann
 * Copyright (c) 2013 - 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/storage_traits.hpp
 *
 * This header defines underlying types used as storage
 */

#ifndef BOOST_ATOMIC_DETAIL_STORAGE_TRAITS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_STORAGE_TRAITS_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/string_ops.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename T >
BOOST_FORCEINLINE void non_atomic_load(T const volatile& from, T& to) BOOST_NOEXCEPT
{
    to = from;
}

template< std::size_t Size >
struct BOOST_ATOMIC_DETAIL_MAY_ALIAS buffer_storage
{
    unsigned char data[Size];

    BOOST_FORCEINLINE bool operator! () const BOOST_NOEXCEPT
    {
        return (data[0] == 0u && BOOST_ATOMIC_DETAIL_MEMCMP(data, data + 1, Size - 1u) == 0);
    }

    BOOST_FORCEINLINE bool operator== (buffer_storage const& that) const BOOST_NOEXCEPT
    {
        return BOOST_ATOMIC_DETAIL_MEMCMP(data, that.data, Size) == 0;
    }

    BOOST_FORCEINLINE bool operator!= (buffer_storage const& that) const BOOST_NOEXCEPT
    {
        return BOOST_ATOMIC_DETAIL_MEMCMP(data, that.data, Size) != 0;
    }
};

template< std::size_t Size >
BOOST_FORCEINLINE void non_atomic_load(buffer_storage< Size > const volatile& from, buffer_storage< Size >& to) BOOST_NOEXCEPT
{
    BOOST_ATOMIC_DETAIL_MEMCPY(to.data, const_cast< unsigned char const* >(from.data), Size);
}

template< std::size_t Size >
struct storage_traits
{
    typedef buffer_storage< Size > type;

    // By default, use the maximum supported alignment
    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 16u;
};

template< >
struct storage_traits< 1u >
{
    typedef boost::uint8_t BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 1u;
};

template< >
struct storage_traits< 2u >
{
    typedef boost::uint16_t BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 2u;
};

template< >
struct storage_traits< 4u >
{
    typedef boost::uint32_t BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 4u;
};

template< >
struct storage_traits< 8u >
{
    typedef boost::uint64_t BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 8u;
};

#if defined(BOOST_HAS_INT128)

template< >
struct storage_traits< 16u >
{
    typedef boost::uint128_type BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 16u;
};

#elif !defined(BOOST_NO_ALIGNMENT)

struct BOOST_ATOMIC_DETAIL_MAY_ALIAS storage128_t
{
    typedef boost::uint64_t BOOST_ATOMIC_DETAIL_MAY_ALIAS element_type;

    BOOST_ALIGNMENT(16) element_type data[2u];

    BOOST_FORCEINLINE bool operator! () const BOOST_NOEXCEPT
    {
        return (data[0] | data[1]) == 0u;
    }
};

BOOST_FORCEINLINE bool operator== (storage128_t const& left, storage128_t const& right) BOOST_NOEXCEPT
{
    return ((left.data[0] ^ right.data[0]) | (left.data[1] ^ right.data[1])) == 0u;
}
BOOST_FORCEINLINE bool operator!= (storage128_t const& left, storage128_t const& right) BOOST_NOEXCEPT
{
    return !(left == right);
}

BOOST_FORCEINLINE void non_atomic_load(storage128_t const volatile& from, storage128_t& to) BOOST_NOEXCEPT
{
    to.data[0] = from.data[0];
    to.data[1] = from.data[1];
}

template< >
struct storage_traits< 16u >
{
    typedef storage128_t type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 16u;
};

#endif

template< typename T >
struct storage_size_of
{
    static BOOST_CONSTEXPR_OR_CONST std::size_t size = sizeof(T);
    static BOOST_CONSTEXPR_OR_CONST std::size_t value = (size == 3u ? 4u : (size >= 5u && size <= 7u ? 8u : (size >= 9u && size <= 15u ? 16u : size)));
};

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_STORAGE_TRAITS_HPP_INCLUDED_
