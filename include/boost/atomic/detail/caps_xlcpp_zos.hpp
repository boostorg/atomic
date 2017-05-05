/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2017 Dynatrace
 */
/*!
 * \file   atomic/detail/caps_xlcpp_zos.hpp
 *
 * This header defines feature capabilities macros
 */

#ifndef BOOST_ATOMIC_DETAIL_CAPS_XLCPP_ZOS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CAPS_XLCPP_ZOS_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__64BIT__) && (__ARCH__ >= 5)
#define BOOST_ATOMIC_DETAIL_XLCPP_ZOS_HAS_GRANDE_CAS
#endif

#define BOOST_ATOMIC_INT8_LOCK_FREE 2
#define BOOST_ATOMIC_INT16_LOCK_FREE 2
#define BOOST_ATOMIC_INT32_LOCK_FREE 2
#define BOOST_ATOMIC_INT64_LOCK_FREE 2

#if defined(BOOST_ATOMIC_DETAIL_XLCPP_ZOS_HAS_GRANDE_CAS) && (defined(BOOST_HAS_INT128) || !defined(BOOST_NO_ALIGNMENT))
#define BOOST_ATOMIC_INT128_LOCK_FREE 2
#endif

#define BOOST_ATOMIC_POINTER_LOCK_FREE 2

#define BOOST_ATOMIC_THREAD_FENCE 2
#define BOOST_ATOMIC_SIGNAL_FENCE 2

#endif // BOOST_ATOMIC_DETAIL_CAPS_XLCPP_ZOS_HPP_INCLUDED_
