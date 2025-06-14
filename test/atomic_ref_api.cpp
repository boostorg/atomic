//  Copyright (c) 2020-2025 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic/atomic_ref.hpp>
#include <boost/memory_order.hpp>

#include <cstdint>
#include <boost/config.hpp>

#include "aligned_object.hpp"
#include "atomic_wrapper.hpp"
#include "api_test_helpers.hpp"

int main(int, char *[])
{
    test_integral_api< atomic_ref_wrapper, char >();
    test_integral_api< atomic_ref_wrapper, signed char >();
    test_integral_api< atomic_ref_wrapper, unsigned char >();
    test_integral_api< atomic_ref_wrapper, std::uint8_t >();
    test_integral_api< atomic_ref_wrapper, std::int8_t >();
    test_integral_api< atomic_ref_wrapper, short >();
    test_integral_api< atomic_ref_wrapper, unsigned short >();
    test_integral_api< atomic_ref_wrapper, std::uint16_t >();
    test_integral_api< atomic_ref_wrapper, std::int16_t >();
    test_integral_api< atomic_ref_wrapper, int >();
    test_integral_api< atomic_ref_wrapper, unsigned int >();
    test_integral_api< atomic_ref_wrapper, std::uint32_t >();
    test_integral_api< atomic_ref_wrapper, std::int32_t >();
    test_integral_api< atomic_ref_wrapper, long >();
    test_integral_api< atomic_ref_wrapper, unsigned long >();
    test_integral_api< atomic_ref_wrapper, std::uint64_t >();
    test_integral_api< atomic_ref_wrapper, std::int64_t >();
    test_integral_api< atomic_ref_wrapper, long long >();
    test_integral_api< atomic_ref_wrapper, unsigned long long >();
#if defined(BOOST_HAS_INT128) && !defined(BOOST_ATOMIC_TESTS_NO_INT128)
    test_integral_api< atomic_ref_wrapper, boost::int128_type >();
    test_integral_api< atomic_ref_wrapper, boost::uint128_type >();
#endif

#if !defined(BOOST_ATOMIC_NO_FLOATING_POINT)
    test_floating_point_api< atomic_ref_wrapper, float >();
    test_floating_point_api< atomic_ref_wrapper, double >();
    test_floating_point_api< atomic_ref_wrapper, long double >();
#if defined(BOOST_HAS_FLOAT128) && !defined(BOOST_ATOMIC_TESTS_NO_FLOAT128)
    test_floating_point_api< atomic_ref_wrapper, boost::float128_type >();
#endif
#endif

    test_pointer_api< atomic_ref_wrapper, int >();

    test_enum_api< atomic_ref_wrapper >();

    test_struct_api< atomic_ref_wrapper, test_struct< std::uint8_t > >();
    test_struct_api< atomic_ref_wrapper, test_struct< std::uint16_t > >();
    test_struct_api< atomic_ref_wrapper, test_struct< std::uint32_t > >();
    test_struct_api< atomic_ref_wrapper, test_struct< std::uint64_t > >();
#if defined(BOOST_HAS_INT128)
    test_struct_api< atomic_ref_wrapper, test_struct< boost::uint128_type > >();
#endif

    // https://svn.boost.org/trac/boost/ticket/10994
    test_struct_x2_api< atomic_ref_wrapper, test_struct_x2< std::uint64_t > >();

    // https://svn.boost.org/trac/boost/ticket/9985
    test_struct_api< atomic_ref_wrapper, test_struct< double > >();

    test_large_struct_api< atomic_ref_wrapper >();

    // Test that boost::atomic_ref<T> only requires T to be trivially copyable.
    // Other non-trivial constructors are allowed.
    test_struct_with_ctor_api< atomic_ref_wrapper >();

#if !defined(BOOST_ATOMIC_NO_CLEAR_PADDING)
    test_struct_with_padding_api< atomic_ref_wrapper >();
#endif

    {
        aligned_object< const int, boost::atomic_ref< const int >::required_alignment > object(10);
        boost::atomic_ref< const int > r(object.get());
        BOOST_TEST_EQ(r.load(boost::memory_order_relaxed), 10);
    }
#if !defined(BOOST_NO_CXX17_DEDUCTION_GUIDES)
    if (boost::atomic_ref< int >::is_always_lock_free)
    {
        aligned_object< int, boost::atomic_ref< int >::required_alignment > object(0);
        boost::atomic_ref r(object.get());
        r.store(1, boost::memory_order_relaxed);
    }
#endif
    if (boost::atomic_ref< int >::is_always_lock_free)
    {
        aligned_object< int, boost::atomic_ref< int >::required_alignment > object(0);
        boost::atomic_ref< int > r = boost::make_atomic_ref(object.get());
        r.store(1, boost::memory_order_relaxed);
    }

    return boost::report_errors();
}
