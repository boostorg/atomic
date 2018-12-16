//  Copyright (c) 2018 Mike Dev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic.hpp>

struct Dummy {
    int a, b, c, d;
};

int main() {
    Dummy d = { 1,2,3,4 };
    boost::atomic<Dummy> ad;

    // this operation requires functions from
    // the compiled part of Boost.Atomic
    ad = d;
}
