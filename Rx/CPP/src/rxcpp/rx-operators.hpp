// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OPERATORS_HPP)
#define RXCPP_RX_OPERATORS_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace operators {

struct tag_operator {};
template<class T>
struct operator_base
{
    typedef T value_type;
    typedef tag_operator operator_tag;
};
template<class T>
class is_operator
{
    template<class C>
    static typename C::operator_tag check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_operator>::value;
};

}
namespace rxo=operators;

}

#include "operators/rx-subscribe.hpp"
#include "operators/rx-filter.hpp"
#include "operators/rx-map.hpp"
#include "operators/rx-flat_map.hpp"

#endif
