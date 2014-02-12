// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_HPP)
#define RXCPP_RX_HPP

#include "rx-includes.hpp"

namespace rxcpp {

template<class T, class SourceOperator>
class observable
{
    SourceOperator source_operator;

private:
    template<class I>
    auto detail_subscribe(observer<T, I> o, tag_observer&&)
        -> decltype(make_subscription(o)) {

        auto subscriber = [=]() {
            try {
                source_operator.on_subscribe(o);
            }
            catch(...) {
                if (!o.is_subscribed()) {
                    throw;
                }
                o.on_error(std::current_exception());
                o.unsubscribe();
            }
        };

        if (rxsc::current_thread::is_schedule_required()) {
            auto sc = rxsc::make_current_thread();
            sc->schedule([=](rxsc::action, rxsc::scheduler) {
                subscriber();
                return rxsc::make_action_empty();
            });
        } else {
            subscriber();
        }

        return make_subscription(o);
    }

    struct tag_onnext {};
    template<class OnNext>
    auto detail_subscribe(OnNext n, tag_onnext&&)
        -> decltype(make_subscription(make_observer<T>(std::move(n)))) {
        return subscribe(make_observer<T>(std::move(n)));
    }

public:
    typedef T value_type;

    static_assert(rxo::is_operator<SourceOperator>::value || rxs::is_source<SourceOperator>::value, "observable must wrap an operator or source");

    explicit observable(const SourceOperator& o)
        : source_operator(o)
    {}
    explicit observable(SourceOperator&& o)
        : source_operator(std::move(o))
    {}

    template<class Arg>
    auto subscribe(Arg a)
        -> decltype(detail_subscribe(std::move(a), typename std::conditional<is_observer<Arg>::value, tag_observer, tag_onnext>::type())) {
        return      detail_subscribe(std::move(a), typename std::conditional<is_observer<Arg>::value, tag_observer, tag_onnext>::type());
    }

    template<class OnNext, class OnError>
    auto subscribe(OnNext n, OnError e)
        -> decltype(make_subscription(make_observer<T>(std::move(n), std::move(e)))) {
        return subscribe(make_observer<T>(std::move(n), std::move(e)));
    }

    template<class OnNext, class OnError, class OnCompleted>
    auto subscribe(OnNext n, OnError e, OnCompleted c)
        -> decltype(make_subscription(make_observer<T>(std::move(n), std::move(e), std::move(c)))) {
        return subscribe(make_observer<T>(std::move(n), std::move(e), std::move(c)));
    }

    template<class Predicate>
    auto filter(Predicate p)
        ->      observable<T,   rxo::detail::filter<T, observable, Predicate>> {
        return  observable<T,   rxo::detail::filter<T, observable, Predicate>>(
                                rxo::detail::filter<T, observable, Predicate>(*this, std::move(p)));
    }
};

// observable<> has static methods to construct observable sources and adaptors.
// observable<> is not constructable
template<>
class observable<void, void>
{
    ~observable();
public:
    template<class T>
    static auto range(T start = 0, size_t count = std::numeric_limits<size_t>::max(), ptrdiff_t step = 1, rxsc::scheduler sc = rxsc::make_current_thread())
        ->      observable<T,   rxs::detail::range<T>> {
        return  observable<T,   rxs::detail::range<T>>(
                                rxs::detail::range<T>(start, count, step, sc));
    }
};

}

template<class T, class SourceOperator, class OperatorFactory>
auto operator >> (rxcpp::observable<T, SourceOperator> source, OperatorFactory&& op)
    -> decltype(op(source)){
    return      op(source);
}

#endif
