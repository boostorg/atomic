/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2012 Tim Blechmann
 * Copyright (c) 2013-2018, 2020-2025 Andrey Semashev
 */
/*!
 * \file   atomic/detail/bitwise_cast.hpp
 *
 * This header defines \c bitwise_cast used to convert between storage and value types
 */

#ifndef BOOST_ATOMIC_DETAIL_BITWISE_CAST_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_BITWISE_CAST_HPP_INCLUDED_

#include <cstddef>
#include <type_traits>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/addressof.hpp>
#include <boost/atomic/detail/string_ops.hpp>
#include <boost/atomic/detail/type_traits/is_trivially_copyable.hpp>
#include <boost/atomic/detail/type_traits/has_unique_object_representations.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined(BOOST_ATOMIC_DETAIL_NO_HAS_UNIQUE_OBJECT_REPRESENTATIONS)

#if defined(__has_builtin)
#if __has_builtin(__builtin_bit_cast)
#define BOOST_ATOMIC_DETAIL_BIT_CAST(x, y) __builtin_bit_cast(x, y)
#endif
#endif

#if !defined(BOOST_ATOMIC_DETAIL_BIT_CAST) && defined(BOOST_MSVC) && BOOST_MSVC >= 1926
#define BOOST_ATOMIC_DETAIL_BIT_CAST(x, y) __builtin_bit_cast(x, y)
#endif

#endif // !defined(BOOST_ATOMIC_DETAIL_NO_HAS_UNIQUE_OBJECT_REPRESENTATIONS)

#if !defined(BOOST_ATOMIC_DETAIL_BIT_CAST) || !defined(BOOST_ATOMIC_DETAIL_HAS_BUILTIN_ADDRESSOF)
#define BOOST_ATOMIC_DETAIL_NO_CXX11_CONSTEXPR_BITWISE_CAST
#endif

#if !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_CONSTEXPR_BITWISE_CAST)
#define BOOST_ATOMIC_DETAIL_CONSTEXPR_BITWISE_CAST constexpr
#else
#define BOOST_ATOMIC_DETAIL_CONSTEXPR_BITWISE_CAST
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
BOOST_FORCEINLINE void clear_tail_padding_bits(To& to, std::true_type) noexcept
{
    BOOST_ATOMIC_DETAIL_MEMSET(reinterpret_cast< unsigned char* >(atomics::detail::addressof(to)) + ValueSize, 0, sizeof(To) - ValueSize);
}

template< std::size_t ValueSize, typename To >
BOOST_FORCEINLINE void clear_tail_padding_bits(To&, std::false_type) noexcept
{
}

template< std::size_t ValueSize, typename To >
BOOST_FORCEINLINE void clear_tail_padding_bits(To& to) noexcept
{
    atomics::detail::clear_tail_padding_bits< ValueSize >(to, std::integral_constant< bool, ValueSize < sizeof(To) >());
}

template< typename To, std::size_t FromValueSize, typename From >
BOOST_FORCEINLINE To bitwise_cast_memcpy(From const& from) noexcept
{
    using unqualified_to_t = typename std::remove_cv< To >::type;
    static_assert(atomics::detail::is_trivially_copyable< unqualified_to_t >::value, "bitwise_cast target type must be trivially copyable");
    static_assert(atomics::detail::is_trivially_copyable< From >::value, "bitwise_cast source type must be trivially copyable");

    // Suppress default constructor of To as it may potentially be a non-trivial throwing constructor
    union cast_helper
    {
        unsigned char as_bytes[sizeof(unqualified_to_t)];
        unqualified_to_t as_to;

        BOOST_FORCEINLINE cast_helper() noexcept {}
    }
    storage;

#if !defined(BOOST_ATOMIC_NO_CLEAR_PADDING)
    From from2(from);
    BOOST_ATOMIC_DETAIL_CLEAR_PADDING(atomics::detail::addressof(from2));
    BOOST_ATOMIC_DETAIL_MEMCPY
    (
        atomics::detail::addressof(storage.as_to),
        atomics::detail::addressof(from2),
        (FromValueSize < sizeof(unqualified_to_t) ? FromValueSize : sizeof(unqualified_to_t))
    );
#else
    BOOST_ATOMIC_DETAIL_MEMCPY
    (
        atomics::detail::addressof(storage.as_to),
        atomics::detail::addressof(from),
        (FromValueSize < sizeof(unqualified_to_t) ? FromValueSize : sizeof(unqualified_to_t))
    );
#endif
    atomics::detail::clear_tail_padding_bits< FromValueSize >(storage.as_to);
    return storage.as_to;
}

#if defined(BOOST_ATOMIC_DETAIL_BIT_CAST)

template< typename To, std::size_t FromValueSize, typename From >
BOOST_FORCEINLINE BOOST_ATOMIC_DETAIL_CONSTEXPR_BITWISE_CAST To bitwise_cast_impl(From const& from, std::true_type) noexcept
{
    // This implementation is only called when the From type has no padding and From and To have the same size
    return BOOST_ATOMIC_DETAIL_BIT_CAST(typename std::remove_cv< To >::type, from);
}

template< typename To, std::size_t FromValueSize, typename From >
BOOST_FORCEINLINE To bitwise_cast_impl(From const& from, std::false_type) noexcept
{
    return atomics::detail::bitwise_cast_memcpy< To, FromValueSize >(from);
}

template< typename To, std::size_t FromValueSize, typename From >
BOOST_FORCEINLINE BOOST_ATOMIC_DETAIL_CONSTEXPR_BITWISE_CAST To bitwise_cast(From const& from) noexcept
{
    return atomics::detail::bitwise_cast_impl< To, FromValueSize >(from, std::integral_constant< bool,
        FromValueSize == sizeof(To) && atomics::detail::has_unique_object_representations< From >::value >());
}

#else // defined(BOOST_ATOMIC_DETAIL_BIT_CAST)

template< typename To, std::size_t FromValueSize, typename From >
BOOST_FORCEINLINE To bitwise_cast(From const& from) noexcept
{
    return atomics::detail::bitwise_cast_memcpy< To, FromValueSize >(from);
}

#endif // defined(BOOST_ATOMIC_DETAIL_BIT_CAST)

//! Converts the source object to the target type, possibly by padding or truncating it on the right, and clearing any padding bits (if supported by compiler).
//! Preserves value bits unchanged.
template< typename To, typename From >
BOOST_FORCEINLINE BOOST_ATOMIC_DETAIL_CONSTEXPR_BITWISE_CAST To bitwise_cast(From const& from) noexcept
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
