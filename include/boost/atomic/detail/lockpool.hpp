#ifndef BOOST_ATOMIC_DETAIL_LOCKPOOL_HPP
#define BOOST_ATOMIC_DETAIL_LOCKPOOL_HPP

//  Copyright (c) 2011 Helge Bahmann
//  Copyright (c) 2013 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/link.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

#if !defined(BOOST_ATOMIC_FLAG_LOCK_FREE) || BOOST_ATOMIC_FLAG_LOCK_FREE != 2

class lockpool
{
public:
    class scoped_lock
    {
        void* lock_;

    public:
        explicit BOOST_ATOMIC_DECL scoped_lock(const volatile void* addr);
        BOOST_ATOMIC_DECL ~scoped_lock();

        BOOST_DELETED_FUNCTION(scoped_lock(scoped_lock const&))
        BOOST_DELETED_FUNCTION(scoped_lock& operator=(scoped_lock const&))
    };
};

#else

class lockpool
{
public:
    typedef atomic_flag lock_type;

    class scoped_lock
    {
    private:
        lock_type& flag_;

    public:
        explicit
        scoped_lock(const volatile void * addr) : flag_(get_lock_for(addr))
        {
            while (flag_.test_and_set(memory_order_acquire))
            {
#if defined(BOOST_ATOMIC_X86_PAUSE)
                BOOST_ATOMIC_X86_PAUSE();
#endif
            }
        }

        ~scoped_lock(void)
        {
            flag_.clear(memory_order_release);
        }

        BOOST_DELETED_FUNCTION(scoped_lock(const scoped_lock &))
        BOOST_DELETED_FUNCTION(scoped_lock& operator=(const scoped_lock &))
    };

private:
    static BOOST_ATOMIC_DECL lock_type& get_lock_for(const volatile void * addr);
};

#endif

}
}
}

#endif
