//  Copyright (c) 2011 Helge Bahmann
//  Copyright (c) 2020-2025 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic.hpp>

#include <cstdint>
#include <boost/config.hpp>

#include "atomic_wrapper.hpp"
#include "api_test_helpers.hpp"

int main(int, char *[])
{
    test_flag_api< boost::atomic_flag >();

    test_integral_api< atomic_wrapper, char >();
    test_integral_api< atomic_wrapper, signed char >();
    test_integral_api< atomic_wrapper, unsigned char >();
    test_integral_api< atomic_wrapper, std::uint8_t >();
    test_integral_api< atomic_wrapper, std::int8_t >();
    test_integral_api< atomic_wrapper, short >();
    test_integral_api< atomic_wrapper, unsigned short >();
    test_integral_api< atomic_wrapper, std::uint16_t >();
    test_integral_api< atomic_wrapper, std::int16_t >();
    test_integral_api< atomic_wrapper, int >();
    test_integral_api< atomic_wrapper, unsigned int >();
    test_integral_api< atomic_wrapper, std::uint32_t >();
    test_integral_api< atomic_wrapper, std::int32_t >();
    test_integral_api< atomic_wrapper, long >();
    test_integral_api< atomic_wrapper, unsigned long >();
    test_integral_api< atomic_wrapper, std::uint64_t >();
    test_integral_api< atomic_wrapper, std::int64_t >();
    test_integral_api< atomic_wrapper, long long >();
    test_integral_api< atomic_wrapper, unsigned long long >();
#if defined(BOOST_HAS_INT128) && !defined(BOOST_ATOMIC_TESTS_NO_INT128)
    test_integral_api< atomic_wrapper, boost::int128_type >();
    test_integral_api< atomic_wrapper, boost::uint128_type >();
#endif

    test_constexpr_ctor< bool >();
    test_constexpr_ctor< char >();
    test_constexpr_ctor< short >();
    test_constexpr_ctor< int >();
    test_constexpr_ctor< long >();
    test_constexpr_ctor< long long >();
    test_constexpr_ctor< test_enum >();
#if !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_CONSTEXPR_BITWISE_CAST)
    // As of gcc 11, clang 12 and MSVC 19.27, compilers don't support __builtin_bit_cast from pointers in constant expressions.
    // test_constexpr_ctor< int* >();
    test_constexpr_ctor< test_struct< int > >();
#if !defined(BOOST_ATOMIC_NO_FLOATING_POINT)
    test_constexpr_ctor< float >();
    test_constexpr_ctor< double >();
    // We don't test long double as it may include padding bits, which will make the constructor non-constexpr
#endif
#endif

#if !defined(BOOST_ATOMIC_NO_FLOATING_POINT)
    test_floating_point_api< atomic_wrapper, float >();
    test_floating_point_api< atomic_wrapper, double >();
    test_floating_point_api< atomic_wrapper, long double >();
#if defined(BOOST_HAS_FLOAT128) && !defined(BOOST_ATOMIC_TESTS_NO_FLOAT128)
    test_floating_point_api< atomic_wrapper, boost::float128_type >();
#endif
#endif

    test_pointer_api< atomic_wrapper, int >();

    test_enum_api< atomic_wrapper >();

    test_struct_api< atomic_wrapper, test_struct< std::uint8_t > >();
    test_struct_api< atomic_wrapper, test_struct< std::uint16_t > >();
    test_struct_api< atomic_wrapper, test_struct< std::uint32_t > >();
    test_struct_api< atomic_wrapper, test_struct< std::uint64_t > >();
#if defined(BOOST_HAS_INT128)
    test_struct_api< atomic_wrapper, test_struct< boost::uint128_type > >();
#endif

    // https://svn.boost.org/trac/boost/ticket/10994
    test_struct_x2_api< atomic_wrapper, test_struct_x2< std::uint64_t > >();

    // https://svn.boost.org/trac/boost/ticket/9985
    test_struct_api< atomic_wrapper, test_struct< double > >();

    test_large_struct_api< atomic_wrapper >();

    // Test that boost::atomic<T> only requires T to be trivially copyable.
    // Other non-trivial constructors are allowed.
    test_struct_with_ctor_api< atomic_wrapper >();

#if !defined(BOOST_ATOMIC_NO_CLEAR_PADDING)
    test_struct_with_padding_api< atomic_wrapper >();
#endif

    // Test that fences at least compile
    boost::atomic_thread_fence(boost::memory_order_seq_cst);
    boost::atomic_signal_fence(boost::memory_order_seq_cst);

    boost::atomics::thread_pause();

    return boost::report_errors();
}
