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

    template<class U>
    void construct(const dynamic_observable<U>& o, tag_dynamic_observable&&) {
        state = o.state;
    }

    template<class U>
    void construct(dynamic_observable<U>&& o, tag_dynamic_observable&&) {
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

    typedef tag_dynamic_observable dynamic_observable_tag;

    dynamic_observable()
    {
    }

    template<class SOF>
    explicit dynamic_observable(SOF&& sof)
        : state(std::make_shared<state_type>())
    {
        construct(std::forward<SOF>(sof),
                  typename std::conditional<is_dynamic_observable<SOF>::value, tag_dynamic_observable, typename std::conditional<rxs::is_source<SOF>::value || rxo::is_operator<SOF>::value, rxs::tag_source, tag_function>::type>::type());
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

        // make sure to let current_thread take ownership of the thread as early as possible.
        if (rxsc::current_thread::is_schedule_required()) {
            auto sc = rxsc::make_current_thread();
            sc.schedule(o.get_subscription(), [=](const rxsc::schedulable& scbl) {
                safe_subscribe();
            });
        } else {
            // current_thread already owns this thread.
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

    /// implicit conversion between observables of the same value_type
    template<class SO>
    observable(const observable<T, SO>& o)
        : source_operator(o.source_operator)
    {}
    /// implicit conversion between observables of the same value_type
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

    ///
    /// performs type-forgetting conversion to a new observable
    ///
    observable<T> as_dynamic() {
        return *this;
    }

    ///
    /// subscribe will cause this observable to emit values to the provided subscriber.
    /// callers must provide enough arguments to make a subscriber.
    /// overrides are supported. thus
    ///   subscribe(thesubscriber, composite_subscription())
    /// will take thesubscriber.get_observer() and the provided
    /// subscription and subscribe to the new subscriber.
    /// the on_next, on_error, on_completed methods can be supplied instead of an observer
    /// if a subscription or subscriber is not provided then a new subscription will be created.
    ///
    template<class Arg0, class... ArgN>
    auto subscribe(Arg0&& a0, ArgN&&... an) const
        -> decltype(detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<ArgN>(an)...))) {
        return      detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
    }

    /// filter (AKA Where) ->
    /// for each item from this observable use Predicate to select which items to emit from the new observable that is returned.
    ///
    template<class Predicate>
    auto filter(Predicate&& p) const
        ->      observable<T,   rxo::detail::filter<T, this_type, Predicate>> {
        return  observable<T,   rxo::detail::filter<T, this_type, Predicate>>(
                                rxo::detail::filter<T, this_type, Predicate>(*this, std::forward<Predicate>(p)));
    }

    /// map (AKA Select) ->
    /// for each item from this observable use Selector to produce an item to emit from the new observable that is returned.
    ///
    template<class Selector>
    auto map(Selector&& s) const
        ->      observable<typename rxo::detail::map<this_type, Selector>::value_type, rxo::detail::map<this_type, Selector>> {
        return  observable<typename rxo::detail::map<this_type, Selector>::value_type, rxo::detail::map<this_type, Selector>>(
                                                                                       rxo::detail::map<this_type, Selector>(*this, std::forward<Selector>(s)));
    }

    /// flat_map (AKA SelectMany) ->
    /// for each item from this observable use the CollectionSelector to select an observable and subscribe to that observable.
    /// for each item from all of the selected observables use the ResultSelector to select a value to emit from the new observable that is returned.
    ///
    template<class CollectionSelector, class ResultSelector>
    auto flat_map(CollectionSelector&& s, ResultSelector&& rs) const
        ->      observable<typename rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector>::value_type,  rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector>> {
        return  observable<typename rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector>::value_type,  rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector>>(
                                                                                                                       rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs)));
    }

    /// publish ->
    /// turns a cold observable hot and allows connections to the source to be independent of subscriptions
    ///
    auto publish() const
        ->      connectable_observable<T,   rxo::detail::publish<T, this_type, rxsub::subject<T>>> {
        return  connectable_observable<T,   rxo::detail::publish<T, this_type, rxsub::subject<T>>>(
                                            rxo::detail::publish<T, this_type, rxsub::subject<T>>(*this));
    }

    ///
    /// takes any function that will take this observable and produce a result value.
    /// this is intended to allow externally defined operators to be connected into the expression.
    ///
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

//
// support range() >> filter() >> subscribe() syntax
// '>>' is spelled 'stream'
//
template<class T, class SourceOperator, class OperatorFactory>
auto operator >> (const rxcpp::observable<T, SourceOperator>& source, OperatorFactory&& of)
    -> decltype(source.op(std::forward<OperatorFactory>(of))) {
    return      source.op(std::forward<OperatorFactory>(of));
}

//
// support range() | filter() | subscribe() syntax
// '|' is spelled 'pipe'
//
template<class T, class SourceOperator, class OperatorFactory>
auto operator | (const rxcpp::observable<T, SourceOperator>& source, OperatorFactory&& of)
    -> decltype(source.op(std::forward<OperatorFactory>(of))) {
    return      source.op(std::forward<OperatorFactory>(of));
}

#endif
