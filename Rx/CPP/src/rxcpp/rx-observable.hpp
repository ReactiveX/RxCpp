// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_OBSERVABLE_HPP)
#define RXCPP_RX_SCHEDULER_OBSERVABLE_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace detail {

template<class Source, class F>
struct is_operator_factory_for
{
    struct not_void {};
    template<class CS, class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)(*(CS*)nullptr));
    template<class CS, class CF>
    static not_void check(...);

    typedef decltype(check<Source, F>(0)) detail_result;
    static const bool value = !std::is_same<detail_result, not_void>::value;
};

}

template<class T>
class dynamic_observable
    : public rxs::source_base<T>
{
    struct state_type
        : public std::enable_shared_from_this<state_type>
    {
        typedef std::function<void(observer<T>)> onsubscribe_type;

        onsubscribe_type on_subscribe;
    };
    std::shared_ptr<state_type> state;

    void construct(const dynamic_observable<T>& o, rxs::tag_source&&) {
        state = o.state;
    }

    void construct(dynamic_observable<T>&& o, rxs::tag_source&&) {
        state = std::move(o.state);
    }

    template<class SO>
    void construct(SO so, rxs::tag_source&&) {
        state->on_subscribe = [so](observer<T> o) mutable {
            so.on_subscribe(std::move(o));
        };
    }

    struct tag_function {};
    template<class F>
    void construct(F&& f, tag_function&&) {
        state->on_subscribe = std::forward<F>(f);
    }

public:

    dynamic_observable()
    {
    }

    template<class SOF>
    explicit dynamic_observable(SOF&& sof)
        : state(std::make_shared<state_type>())
    {
        construct(std::forward<SOF>(sof),
            typename std::conditional<rxs::is_source<SOF>::value || rxo::is_operator<SOF>::value, rxs::tag_source, tag_function>::type());
    }

    void on_subscribe(observer<T> o) const {
        state->on_subscribe(std::move(o));
    }

    template<class Observer>
    typename std::enable_if<!std::is_same<typename std::decay<Observer>::type, observer<T>>::value, void>::type
    on_subscribe(Observer o) const {
        auto so = std::make_shared<Observer>(o);
        state->on_subscribe(make_observer_dynamic<T>(
            so->get_subscription(),
        // on_next
            [so](T t){
                so->on_next(t);
            },
        // on_error
            [so](std::exception_ptr e){
                so->on_error(e);
            },
        // on_completed
            [so](){
                so->on_completed();
            }));
    }
};

template<class T, class Source>
observable<T> make_dynamic_observable(Source&& s) {
    return observable<T>(dynamic_observable<T>(std::forward<Source>(s)));
}

template<class T, class SourceOperator>
class observable
{
    typedef observable<T, SourceOperator> this_type;
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
    // implicit conversion between observables of the same value_type
    template<class SO>
    observable(observable<T, SO>&& o)
        : source_operator(std::move(o.source_operator))
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
    observable<T> as_dynamic() {
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

    template<class Predicate>
    auto filter(Predicate p) const
        ->      observable<T,   rxo::detail::filter<T, observable, Predicate>> {
        return  observable<T,   rxo::detail::filter<T, observable, Predicate>>(
                                rxo::detail::filter<T, observable, Predicate>(*this, std::move(p)));
    }

    template<class OperatorFactory>
    auto op(OperatorFactory&& of) const
        -> decltype(of(*(this_type*)nullptr)) {
        static_assert(detail::is_operator_factory_for<this_type, OperatorFactory>::value, "Function passed for op() must have the signature Result(SourceObservable)");
        return      of(*this);
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
auto operator >> (const rxcpp::observable<T, SourceOperator>& source, OperatorFactory&& of)
    -> decltype(source.op(std::forward<OperatorFactory>(of))) {
    return      source.op(std::forward<OperatorFactory>(of));
}

#endif
