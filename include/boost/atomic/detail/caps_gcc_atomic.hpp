/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/caps_gcc_atomic.hpp
 *
 * This header defines feature capabilities macros
 */

#ifndef BOOST_ATOMIC_DETAIL_CAPS_GCC_ATOMIC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CAPS_GCC_ATOMIC_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__i386__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
#define BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B 1
#endif

#if defined(__x86_64__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#define BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B 1
#endif

#if __GCC_ATOMIC_BOOL_LOCK_FREE == 2
#define BOOST_ATOMIC_FLAG_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_CHAR_LOCK_FREE == 2
#define BOOST_ATOMIC_CHAR_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_CHAR16_T_LOCK_FREE == 2
#define BOOST_ATOMIC_CHAR16_T_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_CHAR32_T_LOCK_FREE == 2
#define BOOST_ATOMIC_CHAR32_T_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_WCHAR_T_LOCK_FREE == 2
#define BOOST_ATOMIC_WCHAR_T_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_SHORT_LOCK_FREE == 2
#define BOOST_ATOMIC_SHORT_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_INT_LOCK_FREE == 2
#define BOOST_ATOMIC_INT_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_LONG_LOCK_FREE == 2
#define BOOST_ATOMIC_LONG_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_LLONG_LOCK_FREE == 2
#define BOOST_ATOMIC_LLONG_LOCK_FREE 2
#endif
#if defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B) && (defined(BOOST_HAS_INT128) || !defined(BOOST_NO_ALIGNMENT))
#define BOOST_ATOMIC_INT128_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_POINTER_LOCK_FREE == 2
#define BOOST_ATOMIC_POINTER_LOCK_FREE 2
#endif
#if __GCC_ATOMIC_BOOL_LOCK_FREE == 2
#define BOOST_ATOMIC_BOOL_LOCK_FREE 2
#endif

#define BOOST_ATOMIC_THREAD_FENCE 2
#define BOOST_ATOMIC_SIGNAL_FENCE 2

#endif // BOOST_ATOMIC_DETAIL_CAPS_GCC_ATOMIC_HPP_INCLUDED_
