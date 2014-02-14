// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_HPP)
#define RXCPP_RX_HPP

namespace rxcpp {

template<class T>
struct dynamic_observable;

template<
    class T = void,
    class SourceObservable = std::conditional<std::is_same<T, void>::value,
        void, dynamic_observable<T>>::type>
class observable;

}

#include "rx-includes.hpp"

namespace rxcpp {

template<class T, class SourceOperator>
class observable
{
    mutable SourceOperator source_operator;

private:
    template<class U, class SO>
    friend class observable;

    template<class I>
    auto detail_subscribe(observer<T, I> o, tag_observer&&) const
        -> decltype(make_subscription(o)) {

        if (!o.is_subscribed()) {
            return make_subscription(o);
        }

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

    struct tag_function {};
    template<class OnNext>
    auto detail_subscribe(OnNext n, tag_function&&) const
        -> decltype(make_subscription(  make_observer<T>(std::move(n)))) {
        return subscribe(               make_observer<T>(std::move(n)));
    }

    template<class OnNext, class OnError>
    auto detail_subscribe(OnNext n, OnError e, tag_function&&) const
        -> decltype(make_subscription(  make_observer<T>(std::move(n), std::move(e)))) {
        return subscribe(               make_observer<T>(std::move(n), std::move(e)));
    }

    template<class OnNext, class OnError, class OnCompleted>
    auto detail_subscribe(OnNext n, OnError e, OnCompleted c, tag_function&&) const
        -> decltype(make_subscription(  make_observer<T>(std::move(n), std::move(e), std::move(c)))) {
        return subscribe(               make_observer<T>(std::move(n), std::move(e), std::move(c)));
    }

    template<class OnNext>
    auto detail_subscribe(composite_subscription cs, OnNext n, tag_subscription&&) const
        -> decltype(make_subscription(  make_observer<T>(std::move(cs), std::move(n)))) {
        return subscribe(               make_observer<T>(std::move(cs), std::move(n)));
    }

    template<class OnNext, class OnError>
    auto detail_subscribe(composite_subscription cs, OnNext n, OnError e, tag_subscription&&) const
        -> decltype(make_subscription(  make_observer<T>(std::move(cs), std::move(n), std::move(e)))) {
        return subscribe(               make_observer<T>(std::move(cs), std::move(n), std::move(e)));
    }

public:
    typedef T value_type;

    static_assert(rxo::is_operator<SourceOperator>::value || rxs::is_source<SourceOperator>::value, "observable must wrap an operator or source");

    observable()
    {
    }

    explicit observable(const SourceOperator& o)
        : source_operator(o)
    {
    }
    explicit observable(SourceOperator&& o)
        : source_operator(std::move(o))
    {
    }

    // implicit conversion between observables of the same value_type
    template<class SO>
    observable(const observable<T, SO>& o)
        : source_operator(o.source_operator)
    {}

#if 0
    template<class I>
    void on_subscribe(observer<T, I> o) const {
        source_operator.on_subscribe(o);
    }
#endif

    //
    // performs type-forgetting conversion
    //
    observable<T, dynamic_observable<T>> as_dynamic() {
        return *this;
    }

    template<class Arg>
    auto subscribe(Arg a) const
        -> decltype(detail_subscribe(std::move(a), typename std::conditional<is_observer<Arg>::value, tag_observer, tag_function>::type())) {
        return      detail_subscribe(std::move(a), typename std::conditional<is_observer<Arg>::value, tag_observer, tag_function>::type());
    }

    template<class Arg1, class Arg2>
    auto subscribe(Arg1 a1, Arg2 a2) const
        -> decltype(detail_subscribe(std::move(a1), std::move(a2), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, tag_function>::type())) {
        return      detail_subscribe(std::move(a1), std::move(a2), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, tag_function>::type());
    }

    template<class Arg1, class Arg2, class Arg3>
    auto subscribe(Arg1 a1, Arg2 a2, Arg3 a3) const
        -> decltype(detail_subscribe(std::move(a1), std::move(a2), std::move(a3), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, tag_function>::type())) {
        return      detail_subscribe(std::move(a1), std::move(a2), std::move(a3), typename std::conditional<is_subscription<Arg1>::value, tag_subscription, tag_function>::type());
    }

    template<class OnNext, class OnError, class OnCompleted>
    auto subscribe(composite_subscription cs, OnNext n, OnError e, OnCompleted c) const
        -> decltype(make_subscription(  make_observer<T>(std::move(cs), std::move(n), std::move(e), std::move(c)))) {
        return subscribe(               make_observer<T>(std::move(cs), std::move(n), std::move(e), std::move(c)));
    }

#if RXCPP_USE_OBSERVABLE_MEMBERS
    template<class Predicate>
    auto filter(Predicate p) const
        ->      observable<T,   rxo::detail::filter<T, observable, Predicate>> {
        return  observable<T,   rxo::detail::filter<T, observable, Predicate>>(
                                rxo::detail::filter<T, observable, Predicate>(*this, std::move(p)));
    }
#endif
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
