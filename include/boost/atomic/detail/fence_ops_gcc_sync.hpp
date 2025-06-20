/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020-2025 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_ops_gcc_sync.hpp
 *
 * This header contains implementation of the \c fence_operations struct.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_OPS_GCC_SYNC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_OPS_GCC_SYNC_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Fence operations based on gcc __sync* intrinsics
struct fence_operations_gcc_sync
{
    static BOOST_FORCEINLINE void thread_fence(memory_order order) noexcept
    {
        if (order != memory_order_relaxed)
            __sync_synchronize();
    }

    static BOOST_FORCEINLINE void signal_fence(memory_order order) noexcept
    {
        if (order != memory_order_relaxed)
            __asm__ __volatile__ ("" ::: "memory");
    }
};

using fence_operations = fence_operations_gcc_sync;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FENCE_OPS_GCC_SYNC_HPP_INCLUDED_
