//  Copyright (c) 2020 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ATOMIC_TESTS_ALIGNED_OBJECT_HPP_INCLUDED_
#define BOOST_ATOMIC_TESTS_ALIGNED_OBJECT_HPP_INCLUDED_

#include <new>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/align/aligned_alloc.hpp>

//! A wrapper that creates an object that has at least the specified alignment
template< typename T, std::size_t Alignment >
class aligned_object
{
private:
    T* const m_p;

public:
    aligned_object() :
        m_p(static_cast< T* >(boost::alignment::aligned_alloc(Alignment, sizeof(T))))
    {
        if (BOOST_UNLIKELY(!m_p))
            BOOST_THROW_EXCEPTION(std::bad_alloc());
        new (m_p) T;
    }

    explicit aligned_object(T const& value) :
        m_p(static_cast< T* >(boost::alignment::aligned_alloc(Alignment, sizeof(T))))
    {
        if (BOOST_UNLIKELY(!m_p))
            BOOST_THROW_EXCEPTION(std::bad_alloc());
        new (m_p) T(value);
    }

    ~aligned_object() BOOST_NOEXCEPT
    {
        m_p->~T();
        boost::alignment::aligned_free(m_p);
    }

    T& get() const BOOST_NOEXCEPT
    {
        return *m_p;
    }

    BOOST_DELETED_FUNCTION(aligned_object(aligned_object const&))
    BOOST_DELETED_FUNCTION(aligned_object& operator= (aligned_object const&))
};

#endif // BOOST_ATOMIC_TESTS_ALIGNED_OBJECT_HPP_INCLUDED_
