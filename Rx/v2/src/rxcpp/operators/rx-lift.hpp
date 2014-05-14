// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_LIFT_HPP)
#define RXCPP_OPERATORS_RX_LIFT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace detail {

template<class S, class F>
struct is_lift_function_for {

    struct tag_not_valid {};
    template<class CS, class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)(*(CS*)nullptr));
    template<class CS, class CF>
    static tag_not_valid check(...);

    typedef typename std::decay<S>::type for_type;
    typedef typename std::decay<F>::type func_type;
    typedef decltype(check<for_type, func_type>(0)) detail_result;
    static const bool value = is_subscriber<detail_result>::value && is_subscriber<for_type>::value;
};

}

namespace operators {

namespace detail {

template<class SourceOperator, class Operator>
struct lift_traits
{
    typedef typename std::decay<SourceOperator>::type source_operator_type;
    typedef typename std::decay<Operator>::type operator_type;

    typedef typename source_operator_type::value_type source_value_type;

    static_assert(rxcpp::detail::is_lift_function_for<subscriber<source_value_type>, operator_type>::value, "lift Operator must be a function with the signature subscriber<...>(subscriber<source_value_type, ...>)");

    typedef decltype((*(operator_type*)nullptr)(*(subscriber<source_value_type>*)nullptr)) result_for_dynamic_source_subscriber_type;

    typedef typename result_for_dynamic_source_subscriber_type::value_type result_value_type;
};

template<class SourceOperator, class Operator>
struct lift : public operator_base<typename lift_traits<SourceOperator, Operator>::result_value_type>
{
    typedef lift_traits<SourceOperator, Operator> traits;
    typedef typename traits::source_operator_type source_operator_type;
    typedef typename traits::operator_type operator_type;
    source_operator_type source;
    operator_type chain;

    lift(source_operator_type s, operator_type op)
        : source(std::move(s))
        , chain(std::move(op))
    {
    }
    template<class Subscriber>
    void on_subscribe(const Subscriber& o) const {
        source.on_subscribe(chain(o));
    }
};

}

}

}

#endif
