/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/caps_gcc_ppc.hpp
 *
 * This header defines feature capabilities macros
 */

#ifndef BOOST_ATOMIC_DETAIL_CAPS_GCC_PPC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CAPS_GCC_PPC_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#define BOOST_ATOMIC_FLAG_LOCK_FREE 2
#define BOOST_ATOMIC_CHAR_LOCK_FREE 2
#define BOOST_ATOMIC_CHAR16_T_LOCK_FREE 2
#define BOOST_ATOMIC_CHAR32_T_LOCK_FREE 2
#define BOOST_ATOMIC_WCHAR_T_LOCK_FREE 2
#define BOOST_ATOMIC_SHORT_LOCK_FREE 2
#define BOOST_ATOMIC_INT_LOCK_FREE 2

#if defined(__SIZEOF_LONG__)
#if __SIZEOF_LONG__ == 4
#define BOOST_ATOMIC_LONG_LOCK_FREE 2
#elif __SIZEOF_LONG__ == 8 && defined(__powerpc64__)
#define BOOST_ATOMIC_LONG_LOCK_FREE 2
#endif
#else
#include <limits.h>
#if ULONG_MAX == 0xffffffff
#define BOOST_ATOMIC_LONG_LOCK_FREE 2
#elif ULONG_MAX == 0xffffffffffffffff && defined(__powerpc64__)
#define BOOST_ATOMIC_LONG_LOCK_FREE 2
#endif
#endif

#if defined(__powerpc64__)
#define BOOST_ATOMIC_LLONG_LOCK_FREE 2
#endif

#define BOOST_ATOMIC_POINTER_LOCK_FREE 2
#define BOOST_ATOMIC_BOOL_LOCK_FREE 2

#define BOOST_ATOMIC_THREAD_FENCE 2
#define BOOST_ATOMIC_SIGNAL_FENCE 2

#endif // BOOST_ATOMIC_DETAIL_CAPS_GCC_PPC_HPP_INCLUDED_
