/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/atomic_traits_fwd.hpp
 *
 * This header contains forward declaration of the \c atomic_traits template.
 */

#ifndef BOOST_ATOMIC_DETAIL_ATOMIC_TRAITS_FWD_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_ATOMIC_TRAITS_FWD_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< unsigned int Size >
struct atomic_traits;

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_ATOMIC_TRAITS_FWD_HPP_INCLUDED_
