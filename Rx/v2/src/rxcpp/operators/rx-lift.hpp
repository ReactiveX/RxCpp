// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_LIFT_HPP)
#define RXCPP_OPERATORS_RX_LIFT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace detail {

template<class V, class S, class F>
struct is_lift_function_for {

    struct tag_not_valid {};
    template<class CS, class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)(*(CS*)nullptr));
    template<class CS, class CF>
    static tag_not_valid check(...);

    typedef typename std::decay<S>::type for_type;
    typedef typename std::decay<F>::type func_type;
    typedef decltype(check<for_type, func_type>(0)) detail_result;
    static const bool value = is_subscriber<detail_result>::value && is_subscriber<for_type>::value && std::is_convertible<V, typename detail_result::value_type>::value;
};

}

namespace operators {

namespace detail {

template<class ResultType, class SourceOperator, class Operator>
struct lift_traits
{
    typedef typename std::decay<ResultType>::type result_value_type;
    typedef typename std::decay<SourceOperator>::type source_operator_type;
    typedef typename std::decay<Operator>::type operator_type;

    typedef typename source_operator_type::value_type source_value_type;

    static_assert(rxcpp::detail::is_lift_function_for<source_value_type, subscriber<result_value_type>, operator_type>::value, "lift Operator must be a function with the signature subscriber<source_value_type, ...>(subscriber<result_value_type, ...>)");
};

template<class ResultType, class SourceOperator, class Operator>
struct lift : public operator_base<typename lift_traits<ResultType, SourceOperator, Operator>::result_value_type>
{
    typedef lift_traits<ResultType, SourceOperator, Operator> traits;
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
    void on_subscribe(Subscriber o) const {
        auto lifted = chain(std::move(o));
        trace_activity().lift_enter(source, chain, o, lifted);
        source.on_subscribe(std::move(lifted));
        trace_activity().lift_return(source, chain);
    }
};

template<class ResultType, class Operator>
class lift_factory
{
    typedef typename std::decay<Operator>::type operator_type;
    operator_type chain;
public:
    lift_factory(operator_type op) : chain(std::move(op)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<ResultType>(chain)) {
        return      source.template lift<ResultType>(chain);
    }
};

}

template<class ResultType, class Operator>
auto lift(Operator&& op)
    ->      detail::lift_factory<ResultType, Operator> {
    return  detail::lift_factory<ResultType, Operator>(std::forward<Operator>(op));
}

}

}

#endif
