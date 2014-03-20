// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OBSERVABLE_HPP)
#define RXCPP_RX_OBSERVABLE_HPP

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

    typedef decltype(check<typename std::decay<Source>::type, typename std::decay<F>::type>(0)) detail_result;
    static const bool value = !std::is_same<detail_result, not_void>::value;
};

template<class Subscriber, class T>
struct has_on_subscribe_for
{
    struct not_void {};
    template<class CS, class CT>
    static auto check(int) -> decltype((*(CT*)nullptr).on_subscribe(*(CS*)nullptr));
    template<class CS, class CT>
    static not_void check(...);

    typedef decltype(check<typename std::decay<Subscriber>::type, T>(0)) detail_result;
    static const bool value = std::is_same<detail_result, void>::value;
};

}

template<class T>
class dynamic_observable
    : public rxs::source_base<T>
{
    struct state_type
        : public std::enable_shared_from_this<state_type>
    {
        typedef std::function<void(subscriber<T>)> onsubscribe_type;

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
    void construct(SO&& source, rxs::tag_source&&) {
        typename std::decay<SO>::type so = std::forward<SO>(source);
        state->on_subscribe = [so](subscriber<T> o) mutable {
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

    void on_subscribe(subscriber<T> o) const {
        state->on_subscribe(std::move(o));
    }

    template<class Subscriber>
    typename std::enable_if<!std::is_same<typename std::decay<Subscriber>::type, observer<T>>::value, void>::type
    on_subscribe(Subscriber&& o) const {
        auto so = std::make_shared<typename std::decay<Subscriber>::type>(std::forward<Subscriber>(o));
        state->on_subscribe(make_subscriber<T>(
            *so,
            make_observer_dynamic<T>(
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
                })));
    }
};

template<class T, class Source>
observable<T> make_dynamic_observable(Source&& s) {
    return observable<T>(dynamic_observable<T>(std::forward<Source>(s)));
}

template<class T, class SourceOperator>
class observable
    : public observable_base<T>
{
    static_assert(std::is_same<T, typename SourceOperator::value_type>::value, "SourceOperator::value_type must be the same as T in observable<T, SourceOperator>");

protected:
    typedef observable<T, SourceOperator> this_type;
    typedef typename std::decay<SourceOperator>::type source_operator_type;
    mutable source_operator_type source_operator;

private:
    template<class U, class SO>
    friend class observable;

    template<class Subscriber>
    auto detail_subscribe(Subscriber&& scrbr) const
    -> decltype(make_subscription(*(typename std::decay<Subscriber>::type*)nullptr)) {

        typedef typename std::decay<Subscriber>::type subscriber_type;
        subscriber_type o = std::forward<Subscriber>(scrbr);

        static_assert(is_observer<subscriber_type>::value, "subscribe must be passed an observer");
        static_assert(std::is_same<typename source_operator_type::value_type, T>::value && std::is_convertible<T*, typename subscriber_type::value_type*>::value, "the value types in the sequence must match or be convertible");
        static_assert(detail::has_on_subscribe_for<subscriber_type, source_operator_type>::value, "inner must have on_subscribe method that accepts this subscriber ");

        if (!o.is_subscribed()) {
            return make_subscription(o);
        }

        auto safe_subscribe = [=]() {
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
                safe_subscribe();
                return rxsc::make_action_empty();
            });
        } else {
            safe_subscribe();
        }

        return make_subscription(o);
    }

public:
    typedef T value_type;

    static_assert(rxo::is_operator<source_operator_type>::value || rxs::is_source<source_operator_type>::value, "observable must wrap an operator or source");

    observable()
    {
    }

    explicit observable(const source_operator_type& o)
        : source_operator(o)
    {
    }
    explicit observable(source_operator_type&& o)
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

    template<class Arg0>
    auto subscribe(Arg0&& a0) const
        -> decltype(detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0)))) {
        return      detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0)));
    }

    template<class Arg0, class Arg1>
    auto subscribe(Arg0&& a0, Arg1&& a1) const
        -> decltype(detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1)))) {
        return      detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1)));
    }

    template<class Arg0, class Arg1, class Arg2>
    auto subscribe(Arg0&& a0, Arg1&& a1, Arg2&& a2) const
        -> decltype(detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))) {
        return      detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)));
    }

    template<class Arg0, class Arg1, class Arg2, class Arg3>
    auto subscribe(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3) const
        -> decltype(detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))) {
        return      detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)));
    }

    template<class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
    auto subscribe(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4) const
        -> decltype(detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)))) {
        return      detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)));
    }

    template<class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
    auto subscribe(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5) const
        -> decltype(detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)))) {
        return      detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)));
    }

    template<class Predicate>
    auto filter(Predicate&& p) const
        ->      observable<T,   rxo::detail::filter<T, observable, Predicate>> {
        return  observable<T,   rxo::detail::filter<T, observable, Predicate>>(
                                rxo::detail::filter<T, observable, Predicate>(*this, std::forward<Predicate>(p)));
    }

    template<class Selector>
    auto map(Selector&& s) const
        ->      observable<typename rxo::detail::map<observable, Selector>::value_type, rxo::detail::map<observable, Selector>> {
        return  observable<typename rxo::detail::map<observable, Selector>::value_type, rxo::detail::map<observable, Selector>>(
                                                                                        rxo::detail::map<observable, Selector>(*this, std::forward<Selector>(s)));
    }

    template<class CollectionSelector, class ResultSelector>
    auto flat_map(CollectionSelector&& s, ResultSelector&& rs) const
        ->      observable<typename rxo::detail::flat_map<observable, CollectionSelector, ResultSelector>::value_type,  rxo::detail::flat_map<observable, CollectionSelector, ResultSelector>> {
        return  observable<typename rxo::detail::flat_map<observable, CollectionSelector, ResultSelector>::value_type,  rxo::detail::flat_map<observable, CollectionSelector, ResultSelector>>(
                                                                                                                        rxo::detail::flat_map<observable, CollectionSelector, ResultSelector>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs)));
    }

    template<class OperatorFactory>
    auto op(OperatorFactory&& of) const
        -> decltype(of(*(const this_type*)nullptr)) {
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
