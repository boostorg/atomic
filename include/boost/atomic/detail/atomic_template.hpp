/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/atomic_template.hpp
 *
 * This header contains interface definition of \c atomic template.
 */

#ifndef BOOST_ATOMIC_DETAIL_ATOMIC_TEMPLATE_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_ATOMIC_TEMPLATE_HPP_INCLUDED_

#include <cstddef>
#include <boost/type_traits/is_integral.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/union_cast.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

BOOST_FORCEINLINE BOOST_CONSTEXPR memory_order deduce_failure_order(memory_order order) BOOST_NOEXCEPT
{
    return order == memory_order_acq_rel ? memory_order_acquire : (order == memory_order_release ? memory_order_relaxed : order);
}

template< typename T, bool IsInt = boost::is_integral< T >::value >
struct classify
{
    typedef void type;
};

template< typename T >
struct classify< T, true > { typedef int type; };

template< typename T >
struct classify< T*, false > { typedef void* type; };

template< typename T, typename Kind >
class base_atomic;

//! Implementation for integers
template< typename T >
class base_atomic< T, int >
{
private:
    typedef base_atomic this_type;
    typedef T value_type;
    typedef T difference_type;
    typedef operations< storage_size_of< value_type >::value > operations;

protected:
    typedef value_type value_arg_type;

public:
    typedef typename operations::storage_type storage_type;

protected:
    storage_type m_storage;

public:
    BOOST_DEFAULTED_FUNCTION(base_atomic(), {})
    BOOST_CONSTEXPR explicit base_atomic(value_type v) BOOST_NOEXCEPT : m_storage(v) {}

    void store(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        operations::store(m_storage, static_cast< storage_type >(v), order);
    }

    value_type load(memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        return static_cast< value_type >(operations::load(m_storage, order));
    }

    value_type fetch_add(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return static_cast< value_type >(operations::fetch_add(m_storage, static_cast< storage_type >(v), order));
    }

    value_type fetch_sub(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return static_cast< value_type >(operations::fetch_sub(m_storage, static_cast< storage_type >(v), order));
    }

    value_type exchange(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return static_cast< value_type >(operations::exchange(m_storage, static_cast< storage_type >(v), order));
    }

    bool compare_exchange_strong(
        value_type& expected, value_type desired, memory_order success_order, memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        storage_type old_value = static_cast< storage_type >(expected);
        const bool res = operations::compare_exchange_strong(m_storage, old_value, static_cast< storage_type >(desired), success_order, failure_order);
        expected = static_cast< value_type >(old_value);
        return res;
    }

    bool compare_exchange_strong(value_type& expected, value_type desired, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, order, atomics::detail::deduce_failure_order(order));
    }

    bool compare_exchange_weak(
        value_type& expected, value_type desired, memory_order success_order, memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        storage_type old_value = static_cast< storage_type >(expected);
        const bool res = operations::compare_exchange_weak(m_storage, old_value, static_cast< storage_type >(desired), success_order, failure_order);
        expected = static_cast< value_type >(old_value);
        return res;
    }

    bool compare_exchange_weak(value_type& expected, value_type desired, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_weak(expected, desired, order, atomics::detail::deduce_failure_order(order));
    }

    value_type fetch_and(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return static_cast< value_type >(operations::fetch_and(m_storage, static_cast< storage_type >(v), order));
    }

    value_type fetch_or(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return static_cast< value_type >(operations::fetch_or(m_storage, static_cast< storage_type >(v), order));
    }

    value_type fetch_xor(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return static_cast< value_type >(operations::fetch_xor(m_storage, static_cast< storage_type >(v), order));
    }

    bool is_lock_free() const volatile BOOST_NOEXCEPT
    {
        return operations::is_lock_free(m_storage);
    }

    value_type operator++(int) volatile BOOST_NOEXCEPT
    {
        return fetch_add(1);
    }

    value_type operator++() volatile BOOST_NOEXCEPT
    {
        return fetch_add(1) + 1;
    }

    value_type operator--(int) volatile BOOST_NOEXCEPT
    {
        return fetch_sub(1);
    }

    value_type operator--() volatile BOOST_NOEXCEPT
    {
        return fetch_sub(1) - 1;
    }

    value_type operator+=(difference_type v) volatile BOOST_NOEXCEPT
    {
        return fetch_add(v) + v;
    }

    value_type operator-=(difference_type v) volatile BOOST_NOEXCEPT
    {
        return fetch_sub(v) - v;
    }

    value_type operator&=(value_type v) volatile BOOST_NOEXCEPT
    {
        return fetch_and(v) & v;
    }

    value_type operator|=(value_type v) volatile BOOST_NOEXCEPT
    {
        return fetch_or(v) | v;
    }

    value_type operator^=(value_type v) volatile BOOST_NOEXCEPT
    {
        return fetch_xor(v) ^ v;
    }

    BOOST_DELETED_FUNCTION(base_atomic(base_atomic const&))
    BOOST_DELETED_FUNCTION(base_atomic& operator=(base_atomic const&))
};


//! Implementation for user-defined types, such as structs and enums
template< typename T >
class base_atomic< T, void >
{
private:
    typedef base_atomic this_type;
    typedef T value_type;
    typedef operations< storage_size_of< value_type >::value > operations;

protected:
    typedef value_type const& value_arg_type;

public:
    typedef typename operations::storage_type storage_type;

protected:
    storage_type m_storage;

public:
    BOOST_DEFAULTED_FUNCTION(base_atomic(), {})
    explicit base_atomic(value_type const& v) BOOST_NOEXCEPT : m_storage(atomics::detail::union_cast< storage_type >(v))
    {
    }

    void store(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        operations::store(m_storage, atomics::detail::union_cast< storage_type >(v), order);
    }

    value_type load(memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::load(m_storage, order));
    }

    value_type exchange(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::exchange(m_storage, atomics::detail::union_cast< storage_type >(v), order));
    }

    bool compare_exchange_strong(
        value_type& expected, value_type desired, memory_order success_order, memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        storage_type old_value = atomics::detail::union_cast< storage_type >(expected);
        const bool res = operations::compare_exchange_strong(m_storage, old_value, atomics::detail::union_cast< storage_type >(desired), success_order, failure_order);
        expected = atomics::detail::union_cast< value_type >(old_value);
        return res;
    }

    bool compare_exchange_strong(value_type& expected, value_type desired, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, order, atomics::detail::deduce_failure_order(order));
    }

    bool compare_exchange_weak(
        value_type& expected, value_type desired, memory_order success_order, memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        storage_type old_value = atomics::detail::union_cast< storage_type >(expected);
        const bool res = operations::compare_exchange_weak(m_storage, old_value, atomics::detail::union_cast< storage_type >(desired), success_order, failure_order);
        expected = atomics::detail::union_cast< value_type >(old_value);
        return res;
    }

    bool compare_exchange_weak(value_type& expected, value_type desired, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_weak(expected, desired, order, atomics::detail::deduce_failure_order(order));
    }

    bool is_lock_free() const volatile BOOST_NOEXCEPT
    {
        return operations::is_lock_free(m_storage);
    }

    BOOST_DELETED_FUNCTION(base_atomic(base_atomic const&))
    BOOST_DELETED_FUNCTION(base_atomic& operator=(base_atomic const&))
};


//! Implementation for pointers
template<typename T >
class base_atomic< T*, void* >
{
private:
    typedef base_atomic this_type;
    typedef T* value_type;
    typedef std::ptrdiff_t difference_type;
    typedef operations< storage_size_of< value_type >::value > operations;

protected:
    typedef value_type value_arg_type;

public:
    typedef typename operations::storage_type storage_type;

protected:
    storage_type m_storage;

public:
    BOOST_DEFAULTED_FUNCTION(base_atomic(void), {})
    explicit base_atomic(value_type const& v) BOOST_NOEXCEPT : m_storage(atomics::detail::union_cast< storage_type >(v))
    {
    }

    void store(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        operations::store(m_storage, atomics::detail::union_cast< storage_type >(v), order);
    }

    value_type load(memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::load(m_storage, order));
    }

    value_type fetch_add(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::fetch_add(m_storage, static_cast< storage_type >(v * sizeof(T)), order));
    }

    value_type fetch_sub(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::fetch_sub(m_storage, static_cast< storage_type >(v * sizeof(T)), order));
    }

    value_type exchange(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::exchange(m_storage, atomics::detail::union_cast< storage_type >(v), order));
    }

    bool compare_exchange_strong(
        value_type& expected, value_type desired, memory_order success_order, memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        storage_type old_value = atomics::detail::union_cast< storage_type >(expected);
        const bool res = operations::compare_exchange_strong(m_storage, old_value, atomics::detail::union_cast< storage_type >(desired), success_order, failure_order);
        expected = atomics::detail::union_cast< value_type >(old_value);
        return res;
    }

    bool compare_exchange_strong(value_type& expected, value_type desired, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, order, atomics::detail::deduce_failure_order(order));
    }

    bool compare_exchange_weak(
        value_type& expected, value_type desired, memory_order success_order, memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        storage_type old_value = atomics::detail::union_cast< storage_type >(expected);
        const bool res = operations::compare_exchange_weak(m_storage, old_value, atomics::detail::union_cast< storage_type >(desired), success_order, failure_order);
        expected = atomics::detail::union_cast< value_type >(old_value);
        return res;
    }

    bool compare_exchange_weak(value_type& expected, value_type desired, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_weak(expected, desired, order, atomics::detail::deduce_failure_order(order));
    }

    bool is_lock_free() const volatile BOOST_NOEXCEPT
    {
        return operations::is_lock_free(m_storage);
    }

    value_type operator++(int) volatile BOOST_NOEXCEPT
    {
        return fetch_add(1);
    }

    value_type operator++() volatile BOOST_NOEXCEPT
    {
        return fetch_add(1) + 1;
    }

    value_type operator--(int) volatile BOOST_NOEXCEPT
    {
        return fetch_sub(1);
    }

    value_type operator--() volatile BOOST_NOEXCEPT
    {
        return fetch_sub(1) - 1;
    }

    value_type operator+=(difference_type v) volatile BOOST_NOEXCEPT
    {
        return fetch_add(v) + v;
    }

    value_type operator-=(difference_type v) volatile BOOST_NOEXCEPT
    {
        return fetch_sub(v) - v;
    }

    BOOST_DELETED_FUNCTION(base_atomic(base_atomic const&))
    BOOST_DELETED_FUNCTION(base_atomic& operator=(base_atomic const&))
};


//! Implementation for void pointers
template< >
class base_atomic< void*, void* >
{
private:
    typedef base_atomic this_type;
    typedef void* value_type;
    typedef std::ptrdiff_t difference_type;
    typedef operations< storage_size_of< value_type >::value > operations;

protected:
    typedef value_type value_arg_type;

public:
    typedef typename operations::storage_type storage_type;

protected:
    storage_type m_storage;

public:
    BOOST_DEFAULTED_FUNCTION(base_atomic(void), {})
    explicit base_atomic(value_type const& v) BOOST_NOEXCEPT : m_storage(atomics::detail::union_cast< storage_type >(v))
    {
    }

    void store(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        operations::store(m_storage, atomics::detail::union_cast< storage_type >(v), order);
    }

    value_type load(memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::load(m_storage, order));
    }

    value_type fetch_add(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::fetch_add(m_storage, static_cast< storage_type >(v * sizeof(T)), order));
    }

    value_type fetch_sub(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::fetch_sub(m_storage, static_cast< storage_type >(v * sizeof(T)), order));
    }

    value_type exchange(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return atomics::detail::union_cast< value_type >(operations::exchange(m_storage, atomics::detail::union_cast< storage_type >(v), order));
    }

    bool compare_exchange_strong(
        value_type& expected, value_type desired, memory_order success_order, memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        storage_type old_value = atomics::detail::union_cast< storage_type >(expected);
        const bool res = operations::compare_exchange_strong(m_storage, old_value, atomics::detail::union_cast< storage_type >(desired), success_order, failure_order);
        expected = atomics::detail::union_cast< value_type >(old_value);
        return res;
    }

    bool compare_exchange_strong(value_type& expected, value_type desired, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, order, atomics::detail::deduce_failure_order(order));
    }

    bool compare_exchange_weak(
        value_type& expected, value_type desired, memory_order success_order, memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        storage_type old_value = atomics::detail::union_cast< storage_type >(expected);
        const bool res = operations::compare_exchange_weak(m_storage, old_value, atomics::detail::union_cast< storage_type >(desired), success_order, failure_order);
        expected = atomics::detail::union_cast< value_type >(old_value);
        return res;
    }

    bool compare_exchange_weak(value_type& expected, value_type desired, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_weak(expected, desired, order, atomics::detail::deduce_failure_order(order));
    }

    bool is_lock_free() const volatile BOOST_NOEXCEPT
    {
        return operations::is_lock_free(m_storage);
    }

    value_type operator++(int) volatile BOOST_NOEXCEPT
    {
        return fetch_add(1);
    }

    value_type operator++() volatile BOOST_NOEXCEPT
    {
        return (char*)fetch_add(1) + 1;
    }

    value_type operator--(int) volatile BOOST_NOEXCEPT
    {
        return fetch_sub(1);
    }

    value_type operator--() volatile BOOST_NOEXCEPT
    {
        return (char*)fetch_sub(1) - 1;
    }

    value_type operator+=(difference_type v) volatile BOOST_NOEXCEPT
    {
        return (char*)fetch_add(v) + v;
    }

    value_type operator-=(difference_type v) volatile BOOST_NOEXCEPT
    {
        return (char*)fetch_sub(v) - v;
    }

    BOOST_DELETED_FUNCTION(base_atomic(base_atomic const&))
    BOOST_DELETED_FUNCTION(base_atomic& operator=(base_atomic const&))
};

} // namespace detail

template< typename T >
class atomic :
    public atomics::detail::base_atomic<
        T,
        typename atomics::detail::classify< T >::type
    >
{
private:
    typedef T value_type;
    typedef atomics::detail::base_atomic<
        T,
        typename atomics::detail::classify< T >::type
    > base_type;
    typedef typename base_type::value_arg_type value_arg_type;

public:
    typedef typename base_type::storage_type storage_type;

public:
    BOOST_DEFAULTED_FUNCTION(atomic(), BOOST_NOEXCEPT {})

    // NOTE: The constructor is made explicit because gcc 4.7 complains that
    //       operator=(value_arg_type) is considered ambiguous with operator=(atomic const&)
    //       in assignment expressions, even though conversion to atomic<> is less preferred
    //       than conversion to value_arg_type.
    explicit BOOST_CONSTEXPR atomic(value_arg_type v) BOOST_NOEXCEPT : base_type(v) {}

    BOOST_FORCEINLINE value_type operator= (value_arg_type v) volatile BOOST_NOEXCEPT
    {
        this->store(v);
        return v;
    }

    BOOST_FORCEINLINE operator value_type() volatile const BOOST_NOEXCEPT
    {
        return this->load();
    }

    BOOST_FORCEINLINE storage_type& storage() BOOST_NOEXCEPT { return this->m_storage; }
    BOOST_FORCEINLINE storage_type volatile& storage() volatile BOOST_NOEXCEPT { return this->m_storage; }
    BOOST_FORCEINLINE storage_type const& storage() const BOOST_NOEXCEPT { return this->m_storage; }
    BOOST_FORCEINLINE storage_type const volatile& storage() const volatile BOOST_NOEXCEPT { return this->m_storage; }

    BOOST_DELETED_FUNCTION(atomic(atomic const&))
    BOOST_DELETED_FUNCTION(atomic& operator= (atomic const&) volatile)
};

} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_ATOMIC_TEMPLATE_HPP_INCLUDED_
