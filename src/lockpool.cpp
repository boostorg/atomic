/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2011 Helge Bahmann
 * Copyright (c) 2013-2014 Andrey Semashev
 */
/*!
 * \file   lockpool.cpp
 *
 * This file contains implementation of the lockpool used to emulate atomic ops.
 */

#include <cstddef>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/capabilities.hpp>

#if BOOST_ATOMIC_FLAG_LOCK_FREE == 2
#include <boost/atomic/detail/operations_lockfree.hpp>
#elif !defined(BOOST_HAS_PTHREADS)
#error Boost.Atomic: Unsupported target platform, POSIX threads are required when native atomic operations are not available
#else
#include <pthread.h>
#define BOOST_ATOMIC_USE_PTHREAD
#endif

#include <boost/atomic/detail/lockpool.hpp>
#include <boost/atomic/detail/pause.hpp>

namespace boost {
namespace atomics {
namespace detail {

namespace {

// This seems to be the maximum across all modern CPUs
// NOTE: This constant is made as a macro because some compilers (gcc 4.4 for one) don't allow enums or namespace scope constants in alignment attributes
#define BOOST_ATOMIC_CACHE_LINE_SIZE 64

template< unsigned int N >
struct padding
{
    char data[N];
};
template< >
struct padding< 0 >
{
};

struct BOOST_ALIGNMENT(BOOST_ATOMIC_CACHE_LINE_SIZE) padded_lock
{
#if defined(BOOST_ATOMIC_USE_PTHREAD)
    typedef pthread_mutex_t lock_type;
#else
    typedef atomics::detail::operations< 1u > operations;
    typedef operations::storage_type lock_type;
#endif

    lock_type lock;
    // The additional padding is needed to avoid false sharing between locks
    enum { padding_size = (sizeof(lock_type) <= BOOST_ATOMIC_CACHE_LINE_SIZE ?
        (BOOST_ATOMIC_CACHE_LINE_SIZE - sizeof(lock_type)) :
        (BOOST_ATOMIC_CACHE_LINE_SIZE - sizeof(lock_type) % BOOST_ATOMIC_CACHE_LINE_SIZE)) };
    padding< padding_size > pad;
};

static padded_lock g_lock_pool[41]
#if defined(BOOST_ATOMIC_USE_PTHREAD)
=
{
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER
}
#endif
;

} // namespace


#if !defined(BOOST_ATOMIC_USE_PTHREAD)

// NOTE: This function must NOT be inline. Otherwise MSVC 9 will sometimes generate broken code for modulus operation which result in crashes.
BOOST_ATOMIC_DECL lockpool::scoped_lock::scoped_lock(const volatile void* addr) :
    m_lock(&g_lock_pool[reinterpret_cast< std::size_t >(addr) % (sizeof(g_lock_pool) / sizeof(*g_lock_pool))].lock)
{
    while (padded_lock::operations::test_and_set(*static_cast< padded_lock::lock_type* >(m_lock), memory_order_acquire))
    {
        atomics::detail::pause();
    }
}

BOOST_ATOMIC_DECL lockpool::scoped_lock::~scoped_lock()
{
    padded_lock::operations::clear(*static_cast< padded_lock::lock_type* >(m_lock), memory_order_release);
}

#else // !defined(BOOST_ATOMIC_USE_PTHREAD)

BOOST_ATOMIC_DECL lockpool::scoped_lock::scoped_lock(const volatile void* addr) :
    m_lock(&g_lock_pool[reinterpret_cast< std::size_t >(addr) % (sizeof(g_lock_pool) / sizeof(*g_lock_pool))].lock)
{
    BOOST_VERIFY(pthread_mutex_lock(static_cast< pthread_mutex_t* >(m_lock)) == 0);
}

BOOST_ATOMIC_DECL lockpool::scoped_lock::~scoped_lock()
{
    BOOST_VERIFY(pthread_mutex_unlock(static_cast< pthread_mutex_t* >(m_lock)) == 0);
}

#endif // !defined(BOOST_ATOMIC_USE_PTHREAD)

} // namespace detail
} // namespace atomics
} // namespace boost
