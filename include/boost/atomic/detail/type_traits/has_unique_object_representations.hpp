/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/type_traits/has_unique_object_representations.hpp
 *
 * This header defines \c has_unique_object_representations type trait
 */

#ifndef BOOST_ATOMIC_DETAIL_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#if !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_BASIC_HDR_TYPE_TRAITS)
#include <type_traits>
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

#if defined(__cpp_lib_has_unique_object_representations) && __cpp_lib_has_unique_object_representations >= 201606
using std::has_unique_object_representations;
#else
#define BOOST_ATOMIC_DETAIL_NO_CXX17_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS
#endif

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_HPP_INCLUDED_
