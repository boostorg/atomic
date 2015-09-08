//  Copyright (c) 2011 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ATOMIC_API_TEST_HELPERS_HPP
#define BOOST_ATOMIC_API_TEST_HELPERS_HPP

#include <boost/atomic.hpp>
#include <cstring>
#include <limits>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/type_traits/is_unsigned.hpp>

/* provide helpers that exercise whether the API
functions of "boost::atomic" provide the correct
operational semantic in the case of sequential
execution */

static void
test_flag_api(void)
{
#ifndef BOOST_ATOMIC_NO_ATOMIC_FLAG_INIT
    boost::atomic_flag f = BOOST_ATOMIC_FLAG_INIT;
#else
    boost::atomic_flag f;
#endif

    BOOST_TEST( !f.test_and_set() );
    BOOST_TEST( f.test_and_set() );
    f.clear();
    BOOST_TEST( !f.test_and_set() );
}

template<typename T>
void test_base_operators(T value1, T value2, T value3)
{
    /* explicit load/store */
    {
        boost::atomic<T> a(value1);
        BOOST_TEST( a.load() == value1 );
    }

    {
        boost::atomic<T> a(value1);
        a.store(value2);
        BOOST_TEST( a.load() == value2 );
    }

    /* overloaded assignment/conversion */
    {
        boost::atomic<T> a(value1);
        BOOST_TEST( value1 == a );
    }

    {
        boost::atomic<T> a;
        a = value2;
        BOOST_TEST( value2 == a );
    }

    /* exchange-type operators */
    {
        boost::atomic<T> a(value1);
        T n = a.exchange(value2);
        BOOST_TEST( a.load() == value2 && n == value1 );
    }

    {
        boost::atomic<T> a(value1);
        T expected = value1;
        bool success = a.compare_exchange_strong(expected, value3);
        BOOST_TEST( success );
        BOOST_TEST( a.load() == value3 && expected == value1 );
    }

    {
        boost::atomic<T> a(value1);
        T expected = value2;
        bool success = a.compare_exchange_strong(expected, value3);
        BOOST_TEST( !success );
        BOOST_TEST( a.load() == value1 && expected == value1 );
    }

    {
        boost::atomic<T> a(value1);
        T expected;
        bool success;
        do {
            expected = value1;
            success = a.compare_exchange_weak(expected, value3);
        } while(!success);
        BOOST_TEST( success );
        BOOST_TEST( a.load() == value3 && expected == value1 );
    }

    {
        boost::atomic<T> a(value1);
        T expected;
        bool success;
        do {
            expected = value2;
            success = a.compare_exchange_weak(expected, value3);
            if (expected != value2)
                break;
        } while(!success);
        BOOST_TEST( !success );
        BOOST_TEST( a.load() == value1 && expected == value1 );
    }
}

// T requires an int constructor
template <typename T>
void test_constexpr_ctor()
{
#ifndef BOOST_NO_CXX11_CONSTEXPR
    const T value(0);
    const boost::atomic<T> tester(value);
    BOOST_TEST( tester == value );
#endif
}

template<typename T, typename D, typename AddType>
void test_additive_operators_with_type(T value, D delta)
{
    /* note: the tests explicitly cast the result of any addition
    to the type to be tested to force truncation of the result to
    the correct range in case of overflow */

    /* explicit add/sub */
    {
        boost::atomic<T> a(value);
        T n = a.fetch_add(delta);
        BOOST_TEST( a.load() == T((AddType)value + delta) );
        BOOST_TEST( n == value );
    }

    {
        boost::atomic<T> a(value);
        T n = a.fetch_sub(delta);
        BOOST_TEST( a.load() == T((AddType)value - delta) );
        BOOST_TEST( n == value );
    }

    /* overloaded modify/assign*/
    {
        boost::atomic<T> a(value);
        T n = (a += delta);
        BOOST_TEST( a.load() == T((AddType)value + delta) );
        BOOST_TEST( n == T((AddType)value + delta) );
    }

    {
        boost::atomic<T> a(value);
        T n = (a -= delta);
        BOOST_TEST( a.load() == T((AddType)value - delta) );
        BOOST_TEST( n == T((AddType)value - delta) );
    }

    /* overloaded increment/decrement */
    {
        boost::atomic<T> a(value);
        T n = a++;
        BOOST_TEST( a.load() == T((AddType)value + 1) );
        BOOST_TEST( n == value );
    }

    {
        boost::atomic<T> a(value);
        T n = ++a;
        BOOST_TEST( a.load() == T((AddType)value + 1) );
        BOOST_TEST( n == T((AddType)value + 1) );
    }

    {
        boost::atomic<T> a(value);
        T n = a--;
        BOOST_TEST( a.load() == T((AddType)value - 1) );
        BOOST_TEST( n == value );
    }

    {
        boost::atomic<T> a(value);
        T n = --a;
        BOOST_TEST( a.load() == T((AddType)value - 1) );
        BOOST_TEST( n == T((AddType)value - 1) );
    }

    // Opaque operations
    {
        boost::atomic<T> a(value);
        a.opaque_add(delta);
        BOOST_TEST( a.load() == T((AddType)value + delta) );
    }

    {
        boost::atomic<T> a(value);
        a.opaque_sub(delta);
        BOOST_TEST( a.load() == T((AddType)value - delta) );
    }

    // Modify and test operations
    {
        boost::atomic<T> a((T)0);
        bool f = a.add_and_test((D)0);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == (T)0 );

        f = a.add_and_test((D)1);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(((AddType)0) + ((D)1)) );
    }
    {
        boost::atomic<T> a((T)0);
        bool f = a.add_and_test((std::numeric_limits< D >::max)());
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(((AddType)0) + (std::numeric_limits< D >::max)()) );
    }
    {
        boost::atomic<T> a((T)0);
        bool f = a.add_and_test((std::numeric_limits< D >::min)());
        BOOST_TEST( f == ((std::numeric_limits< D >::min)() == 0) );
        BOOST_TEST( a.load() == T(((AddType)0) + (std::numeric_limits< D >::min)()) );
    }

    {
        boost::atomic<T> a((T)0);
        bool f = a.sub_and_test((D)0);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == (T)0 );

        f = a.sub_and_test((D)1);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(((AddType)0) - ((D)1)) );
    }
    {
        boost::atomic<T> a((T)0);
        bool f = a.sub_and_test((std::numeric_limits< D >::max)());
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(((AddType)0) - (std::numeric_limits< D >::max)()) );
    }
    {
        boost::atomic<T> a((T)0);
        bool f = a.sub_and_test((std::numeric_limits< D >::min)());
        BOOST_TEST( f == ((std::numeric_limits< D >::min)() == 0) );
        BOOST_TEST( a.load() == T(((AddType)0) - (std::numeric_limits< D >::min)()) );
    }
}

template<typename T, typename D>
void test_additive_operators(T value, D delta)
{
    test_additive_operators_with_type<T, D, T>(value, delta);
}

template< typename T >
void test_negation()
{
    {
        boost::atomic<T> a((T)1);
        T n = a.fetch_negate();
        BOOST_TEST( a.load() == (T)-1 );
        BOOST_TEST( n == (T)1 );

        n = a.fetch_negate();
        BOOST_TEST( a.load() == (T)1 );
        BOOST_TEST( n == (T)-1 );
    }
    {
        boost::atomic<T> a((T)1);
        a.opaque_negate();
        BOOST_TEST( a.load() == (T)-1 );

        a.opaque_negate();
        BOOST_TEST( a.load() == (T)1 );
    }
}

template<typename T>
void test_additive_wrap(T value)
{
    {
        boost::atomic<T> a(value);
        T n = a.fetch_add(1) + (T)1;
        BOOST_TEST( a.load() == n );
    }
    {
        boost::atomic<T> a(value);
        T n = a.fetch_sub(1) - (T)1;
        BOOST_TEST( a.load() == n );
    }
}

template<typename T>
void test_bit_operators(T value, T delta)
{
    /* explicit and/or/xor */
    {
        boost::atomic<T> a(value);
        T n = a.fetch_and(delta);
        BOOST_TEST( a.load() == T(value & delta) );
        BOOST_TEST( n == value );
    }

    {
        boost::atomic<T> a(value);
        T n = a.fetch_or(delta);
        BOOST_TEST( a.load() == T(value | delta) );
        BOOST_TEST( n == value );
    }

    {
        boost::atomic<T> a(value);
        T n = a.fetch_xor(delta);
        BOOST_TEST( a.load() == T(value ^ delta) );
        BOOST_TEST( n == value );
    }

    {
        boost::atomic<T> a(value);
        T n = a.fetch_complement();
        BOOST_TEST( a.load() == T(~value) );
        BOOST_TEST( n == value );
    }

    /* overloaded modify/assign */
    {
        boost::atomic<T> a(value);
        T n = (a &= delta);
        BOOST_TEST( a.load() == T(value & delta) );
        BOOST_TEST( n == T(value & delta) );
    }

    {
        boost::atomic<T> a(value);
        T n = (a |= delta);
        BOOST_TEST( a.load() == T(value | delta) );
        BOOST_TEST( n == T(value | delta) );
    }

    {
        boost::atomic<T> a(value);
        T n = (a ^= delta);
        BOOST_TEST( a.load() == T(value ^ delta) );
        BOOST_TEST( n == T(value ^ delta) );
    }

    // Opaque operations
    {
        boost::atomic<T> a(value);
        a.opaque_and(delta);
        BOOST_TEST( a.load() == T(value & delta) );
    }

    {
        boost::atomic<T> a(value);
        a.opaque_or(delta);
        BOOST_TEST( a.load() == T(value | delta) );
    }

    {
        boost::atomic<T> a(value);
        a.opaque_xor(delta);
        BOOST_TEST( a.load() == T(value ^ delta) );
    }

    {
        boost::atomic<T> a(value);
        a.opaque_complement();
        BOOST_TEST( a.load() == T(~value) );
    }

    // Modify and test operations
    {
        boost::atomic<T> a((T)1);
        bool f = a.and_and_test((T)1);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(1) );

        f = a.and_and_test((T)0);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == T(0) );

        f = a.and_and_test((T)0);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == T(0) );
    }

    {
        boost::atomic<T> a((T)0);
        bool f = a.or_and_test((T)0);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == T(0) );

        f = a.or_and_test((T)1);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(1) );

        f = a.or_and_test((T)1);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(1) );
    }

    {
        boost::atomic<T> a((T)0);
        bool f = a.xor_and_test((T)0);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == T(0) );

        f = a.xor_and_test((T)1);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(1) );

        f = a.xor_and_test((T)1);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == T(0) );
    }

    // Bit test and modify operations
    {
        boost::atomic<T> a((T)42);
        bool f = a.bit_test_and_set(0);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(43) );

        f = a.bit_test_and_set(1);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == T(43) );

        f = a.bit_test_and_set(2);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(47) );
    }

    {
        boost::atomic<T> a((T)42);
        bool f = a.bit_test_and_reset(0);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(42) );

        f = a.bit_test_and_reset(1);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == T(40) );

        f = a.bit_test_and_set(2);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(44) );
    }

    {
        boost::atomic<T> a((T)42);
        bool f = a.bit_test_and_complement(0);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(43) );

        f = a.bit_test_and_complement(1);
        BOOST_TEST( f == true );
        BOOST_TEST( a.load() == T(41) );

        f = a.bit_test_and_complement(2);
        BOOST_TEST( f == false );
        BOOST_TEST( a.load() == T(45) );
    }
}

template<typename T>
void do_test_integral_api(boost::false_type)
{
    BOOST_TEST( sizeof(boost::atomic<T>) >= sizeof(T));

    test_base_operators<T>(42, 43, 44);
    test_additive_operators<T, T>(42, 17);
    test_bit_operators<T>((T)0x5f5f5f5f5f5f5f5fULL, (T)0xf5f5f5f5f5f5f5f5ULL);

    /* test for unsigned overflow/underflow */
    test_additive_operators<T, T>((T)-1, 1);
    test_additive_operators<T, T>(0, 1);
    /* test for signed overflow/underflow */
    test_additive_operators<T, T>(((T)-1) >> (sizeof(T) * 8 - 1), 1);
    test_additive_operators<T, T>(1 + (((T)-1) >> (sizeof(T) * 8 - 1)), 1);
}

template<typename T>
void do_test_integral_api(boost::true_type)
{
    do_test_integral_api<T>(boost::false_type());

    test_additive_wrap<T>(0u);
    BOOST_CONSTEXPR_OR_CONST T all_ones = ~(T)0u;
    test_additive_wrap<T>(all_ones);
    BOOST_CONSTEXPR_OR_CONST T max_signed_twos_compl = all_ones >> 1;
    test_additive_wrap<T>(all_ones ^ max_signed_twos_compl);
    test_additive_wrap<T>(max_signed_twos_compl);
}

template<typename T>
inline void test_integral_api(void)
{
    do_test_integral_api<T>(boost::is_unsigned<T>());

    if (boost::is_signed<T>::value)
        test_negation<T>();
}

template<typename T>
void test_pointer_api(void)
{
    BOOST_TEST( sizeof(boost::atomic<T *>) >= sizeof(T *));
    BOOST_TEST( sizeof(boost::atomic<void *>) >= sizeof(T *));

    T values[3];

    test_base_operators<T*>(&values[0], &values[1], &values[2]);
    test_additive_operators<T*>(&values[1], 1);

    test_base_operators<void*>(&values[0], &values[1], &values[2]);

#if defined(BOOST_HAS_INTPTR_T)
    boost::atomic<void *> ptr;
    boost::atomic<boost::intptr_t> integral;
    BOOST_TEST( ptr.is_lock_free() == integral.is_lock_free() );
#endif
}

enum test_enum {
    foo, bar, baz
};

static void
test_enum_api(void)
{
    test_base_operators(foo, bar, baz);
}

template<typename T>
struct test_struct {
    typedef T value_type;
    value_type i;
    inline bool operator==(const test_struct & c) const {return i == c.i;}
    inline bool operator!=(const test_struct & c) const {return i != c.i;}
};

template<typename T>
void
test_struct_api(void)
{
    T a = {1}, b = {2}, c = {3};

    test_base_operators(a, b, c);

    {
        boost::atomic<T> sa;
        boost::atomic<typename T::value_type> si;
        BOOST_TEST( sa.is_lock_free() == si.is_lock_free() );
    }
}

template<typename T>
struct test_struct_x2 {
    typedef T value_type;
    value_type i, j;
    inline bool operator==(const test_struct_x2 & c) const {return i == c.i && j == c.j;}
    inline bool operator!=(const test_struct_x2 & c) const {return i != c.i && j != c.j;}
};

template<typename T>
void
test_struct_x2_api(void)
{
    T a = {1, 1}, b = {2, 2}, c = {3, 3};

    test_base_operators(a, b, c);
}

struct large_struct {
    long data[64];

    inline bool operator==(const large_struct & c) const
    {
        return std::memcmp(data, &c.data, sizeof(data)) == 0;
    }
    inline bool operator!=(const large_struct & c) const
    {
        return std::memcmp(data, &c.data, sizeof(data)) != 0;
    }
};

static void
test_large_struct_api(void)
{
    large_struct a = {{1}}, b = {{2}}, c = {{3}};
    test_base_operators(a, b, c);
}

struct test_struct_with_ctor {
    typedef unsigned int value_type;
    value_type i;
    test_struct_with_ctor() : i(0x01234567) {}
    inline bool operator==(const test_struct_with_ctor & c) const {return i == c.i;}
    inline bool operator!=(const test_struct_with_ctor & c) const {return i != c.i;}
};

static void
test_struct_with_ctor_api(void)
{
    {
        test_struct_with_ctor s;
        boost::atomic<test_struct_with_ctor> sa;
        // Check that the default constructor was called
        BOOST_TEST( sa.load() == s );
    }

    test_struct_with_ctor a, b, c;
    a.i = 1;
    b.i = 2;
    c.i = 3;

    test_base_operators(a, b, c);
}

#endif
