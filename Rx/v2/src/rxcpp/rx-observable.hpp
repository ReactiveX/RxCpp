// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OBSERVABLE_HPP)
#define RXCPP_RX_OBSERVABLE_HPP

#include "rx-includes.hpp"

#ifdef __GNUG__
#define EXPLICIT_THIS this->
#else
#define EXPLICIT_THIS
#endif

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

    typedef typename std::decay<Source>::type source_type;
    typedef typename std::decay<F>::type function_type;

    typedef decltype(check<source_type, function_type>(0)) detail_result;
    static const bool value = !std::is_same<detail_result, not_void>::value && is_observable<source_type>::value;
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

struct lift_function {
template<class SourceObservable, class OperatorFactory>
static auto chain(const SourceObservable& source, OperatorFactory&& of)
    -> decltype(source.lift(std::forward<OperatorFactory>(of))) {
    return      source.lift(std::forward<OperatorFactory>(of));
}
};
struct operator_factory {
template<class SourceObservable, class OperatorFactory>
static auto chain(const SourceObservable& source, OperatorFactory&& of)
    -> decltype(source.op(std::forward<OperatorFactory>(of))) {
    return      source.op(std::forward<OperatorFactory>(of));
}
};
struct not_chainable {
};

template<class T, class SourceObservable, class OperatorFactory>
struct select_chain
{
    typedef
        typename std::conditional<
            rxcpp::detail::is_lift_function_for<subscriber<T>, OperatorFactory>::value,
            lift_function,
            typename std::conditional<
                rxcpp::detail::is_operator_factory_for<SourceObservable, OperatorFactory>::value,
                operator_factory,
                not_chainable
            >::type
        >::type type;
};

}

//
// this type is the default used by operators that subscribe to
// multiple sources. It assumes that the sources are already synchronized
//
struct identity_observable
{
    template<class Observable>
    auto operator()(Observable o)
        -> Observable {
        return      std::move(o);
        static_assert(is_observable<Observable>::value, "only support observables");
    }
};

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
    friend bool operator==(const dynamic_observable<U>&, const dynamic_observable<U>&);

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
    explicit dynamic_observable(SOF&& sof, typename std::enable_if<!is_dynamic_observable<SOF>::value, void**>::type = 0)
        : state(std::make_shared<state_type>())
    {
        construct(std::forward<SOF>(sof),
                  typename std::conditional<rxs::is_source<SOF>::value || rxo::is_operator<SOF>::value, rxs::tag_source, tag_function>::type());
    }

    void on_subscribe(subscriber<T> o) const {
        state->on_subscribe(std::move(o));
    }

    template<class Subscriber>
    typename std::enable_if<!std::is_same<Subscriber, observer<T>>::value, void>::type
    on_subscribe(Subscriber o) const {
        state->on_subscribe(make_subscriber<T>(o, make_observer_dynamic<T>(o.get_observer())));
    }
};

template<class T>
inline bool operator==(const dynamic_observable<T>& lhs, const dynamic_observable<T>& rhs) {
    return lhs.state == rhs.state;
}
template<class T>
inline bool operator!=(const dynamic_observable<T>& lhs, const dynamic_observable<T>& rhs) {
    return !(lhs == rhs);
}

template<class T, class Source>
observable<T> make_observable_dynamic(Source&& s) {
    return observable<T>(dynamic_observable<T>(std::forward<Source>(s)));
}

template<>
class observable<void, void>;


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

    template<class U, class SO>
    friend bool operator==(const observable<U, SO>&, const observable<U, SO>&);

    template<class Subscriber>
    auto detail_subscribe(Subscriber o) const
        -> composite_subscription {

        typedef typename std::decay<Subscriber>::type subscriber_type;

        static_assert(is_observer<subscriber_type>::value, "subscribe must be passed an observer");
        static_assert(std::is_same<typename source_operator_type::value_type, T>::value && std::is_convertible<T*, typename subscriber_type::value_type*>::value, "the value types in the sequence must match or be convertible");
        static_assert(detail::has_on_subscribe_for<subscriber_type, source_operator_type>::value, "inner must have on_subscribe method that accepts this subscriber ");

        if (!o.is_subscribed()) {
            return o.get_subscription();
        }

        auto safe_subscribe = [&]() {
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
            const auto& sc = rxsc::make_current_thread();
            sc.create_worker(o.get_subscription()).schedule(
                [&](const rxsc::schedulable& scbl) {
                    safe_subscribe();
                });
        } else {
            // current_thread already owns this thread.
            safe_subscribe();
        }

        return o.get_subscription();
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
    observable<T> as_dynamic() const {
        return *this;
    }

    ///
    /// takes any function that will take this observable and produce a result value.
    /// this is intended to allow externally defined operators, that use subscribe,
    /// to be connected into the expression.
    ///
    template<class OperatorFactory>
    auto op(OperatorFactory&& of) const
        -> decltype(of(*(const this_type*)nullptr)) {
        return      of(*this);
        static_assert(detail::is_operator_factory_for<this_type, OperatorFactory>::value, "Function passed for op() must have the signature Result(SourceObservable)");
    }

    ///
    /// takes any function that will take a subscriber for this observable and produce a subscriber.
    /// this is intended to allow externally defined operators, that use make_subscriber, to be connected
    /// into the expression.
    ///
    template<class Operator>
    auto lift(Operator&& op) const
        ->      observable<typename rxo::detail::lift<source_operator_type, Operator>::value_type,  rxo::detail::lift<source_operator_type, Operator>> {
        return  observable<typename rxo::detail::lift<source_operator_type, Operator>::value_type,  rxo::detail::lift<source_operator_type, Operator>>(
                                                                                                    rxo::detail::lift<source_operator_type, Operator>(source_operator, std::forward<Operator>(op)));
        static_assert(detail::is_lift_function_for<subscriber<T>, Operator>::value, "Function passed for lift() must have the signature subscriber<...>(subscriber<T, ...>)");
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
        -> composite_subscription {
        return detail_subscribe(make_subscriber<T>(std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
    }

    /// filter (AKA Where) ->
    /// for each item from this observable use Predicate to select which items to emit from the new observable that is returned.
    ///
    template<class Predicate>
    auto filter(Predicate p) const
        -> decltype(EXPLICIT_THIS lift(rxo::detail::filter<T, Predicate>(std::move(p)))) {
        return                    lift(rxo::detail::filter<T, Predicate>(std::move(p)));
    }

    /// map (AKA Select) ->
    /// for each item from this observable use Selector to produce an item to emit from the new observable that is returned.
    ///
    template<class Selector>
    auto map(Selector&& s) const
        -> decltype(EXPLICIT_THIS lift(rxo::detail::map<T, Selector>(std::move(s)))) {
        return                    lift(rxo::detail::map<T, Selector>(std::move(s)));
    }

    /// distinct_until_changed ->
    /// for each item from this observable, filter out repeated values and emit only changes from the new observable that is returned.
    ///
    auto distinct_until_changed() const
        -> decltype(EXPLICIT_THIS lift(rxo::detail::distinct_until_changed<T>())) {
        return                    lift(rxo::detail::distinct_until_changed<T>());
    }

    /// buffer ->
    /// collect count items from this observable and produce a vector of them to emit from the new observable that is returned.
    ///
    auto buffer(int count) const
        -> decltype(EXPLICIT_THIS lift(rxo::detail::buffer_count<T>(count, count))) {
        return                    lift(rxo::detail::buffer_count<T>(count, count));
    }

    /// buffer ->
    /// start a new vector every skip items and collect count items from this observable into each vector to emit from the new observable that is returned.
    ///
    auto buffer(int count, int skip) const
        -> decltype(EXPLICIT_THIS lift(rxo::detail::buffer_count<T>(count, skip))) {
        return                    lift(rxo::detail::buffer_count<T>(count, skip));
    }

    template<class Coordination, bool IsObservable = is_observable<value_type>::value>
    struct merge_result;

    template<class Coordination>
    struct merge_result<Coordination, true>
    {
        typedef
            observable<
                typename this_type::value_type::value_type,
                rxo::detail::merge<observable<typename this_type::value_type>, Coordination>>
        type;
        static type make(const this_type* that, Coordination sf) {
            return type(rxo::detail::merge<observable<typename this_type::value_type>, Coordination>(*that, sf));
        }
    };
    template<class Coordination>
    struct merge_result<Coordination, false>
    {
        typedef this_type type;
        static type make(const this_type* that, Coordination) {
            return *that;
        }
    };

    /// merge ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    auto merge() const
        -> typename merge_result<identity_one_worker>::type {
        return  merge_result<identity_one_worker>::make(this, identity_one_worker(rxsc::make_current_thread()));
    }

    /// merge ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Coordination>
    auto merge(Coordination&& sf) const
        -> typename std::enable_if<is_observable<value_type>::value && !is_observable<Coordination>::value,
                observable<typename rxo::detail::merge<this_type, Coordination>::value_type,    rxo::detail::merge<this_type, Coordination>>>::type {
        return  observable<typename rxo::detail::merge<this_type, Coordination>::value_type,    rxo::detail::merge<this_type, Coordination>>(
                                                                                                rxo::detail::merge<this_type, Coordination>(*this, std::forward<Coordination>(sf)));
    }

    /// merge ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Value0, class... ValueN>
    auto merge(Value0 v0, ValueN... vn) const
        -> typename std::enable_if<rxu::all_true<is_observable<Value0>::value, is_observable<ValueN>::value...>::value, observable<T>>::type {
        return      rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...).merge();
    }

    /// merge ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Coordination, class Value0, class... ValueN>
    auto merge(Coordination&& sf, Value0 v0, ValueN... vn) const
        -> typename std::enable_if<rxu::all_true<is_observable<Value0>::value, is_observable<ValueN>::value...>::value && !is_observable<Coordination>::value, observable<T>>::type {
        return      rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...).merge(std::forward<Coordination>(sf));
    }


    /// flat_map (AKA SelectMany) ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable use the CollectionSelector to select an observable and subscribe to that observable.
    /// for each item from all of the selected observables use the ResultSelector to select a value to emit from the new observable that is returned.
    ///
    template<class CollectionSelector, class ResultSelector>
    auto flat_map(CollectionSelector&& s, ResultSelector&& rs) const
        ->      observable<typename rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>::value_type,  rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>> {
        return  observable<typename rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>::value_type,  rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>(
                                                                                                                                            rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), identity_one_worker(rxsc::make_current_thread())));
    }

    /// flat_map (AKA SelectMany) ->
    /// The coodination is used to synchronize sources from different contexts.
    /// for each item from this observable use the CollectionSelector to select an observable and subscribe to that observable.
    /// for each item from all of the selected observables use the ResultSelector to select a value to emit from the new observable that is returned.
    ///
    template<class CollectionSelector, class ResultSelector, class Coordination>
    auto flat_map(CollectionSelector&& s, ResultSelector&& rs, Coordination&& sf) const
        ->      observable<typename rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>::value_type, rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>> {
        return  observable<typename rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>::value_type, rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>>(
                                                                                                                                    rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), std::forward<Coordination>(sf)));
    }

    template<class Coordination, bool IsObservable = is_observable<value_type>::value>
    struct concat_result;

    template<class Coordination>
    struct concat_result<Coordination, true>
    {
        typedef
            observable<
                typename this_type::value_type::value_type,
                rxo::detail::concat<observable<typename this_type::value_type>, Coordination>>
        type;
        static type make(const this_type* that, Coordination sf) {
            return type(rxo::detail::concat<observable<typename this_type::value_type>, Coordination>(*that, sf));
        }
    };
    template<class Coordination>
    struct concat_result<Coordination, false>
    {
        typedef this_type type;
        static type make(const this_type* that, Coordination) {
            return *that;
        }
    };

    /// concat ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe to one at a time. in the order received.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    auto concat() const
        -> typename concat_result<identity_one_worker>::type {
        return  concat_result<identity_one_worker>::make(this, identity_one_worker(rxsc::make_current_thread()));
    }

    /// concat ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe to one at a time. in the order received.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Coordination>
    auto concat(Coordination&& sf) const
        -> typename std::enable_if<is_observable<value_type>::value && !is_observable<Coordination>::value,
                observable<typename rxo::detail::concat<this_type, Coordination>::value_type,   rxo::detail::concat<this_type, Coordination>>>::type {
        return  observable<typename rxo::detail::concat<this_type, Coordination>::value_type,   rxo::detail::concat<this_type, Coordination>>(
                                                                                                rxo::detail::concat<this_type, Coordination>(*this, std::forward<Coordination>(sf)));
    }

    /// concat ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe to one at a time. in the order received.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Value0, class... ValueN>
    auto concat(Value0 v0, ValueN... vn) const
        -> typename std::enable_if<rxu::all_true<is_observable<Value0>::value, is_observable<ValueN>::value...>::value, observable<T>>::type {
        return      rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...).concat();
    }

    /// concat ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe to one at a time. in the order received.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Coordination, class Value0, class... ValueN>
    auto concat(Coordination&& sf, Value0 v0, ValueN... vn) const
        -> typename std::enable_if<rxu::all_true<is_observable<Value0>::value, is_observable<ValueN>::value...>::value && !is_observable<Coordination>::value, observable<T>>::type {
        return      rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...).concat(std::forward<Coordination>(sf));
    }
    /// concat_map ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable use the CollectionSelector to select an observable and subscribe to that observable.
    /// for each item from all of the selected observables use the ResultSelector to select a value to emit from the new observable that is returned.
    ///
    template<class CollectionSelector, class ResultSelector>
    auto concat_map(CollectionSelector&& s, ResultSelector&& rs) const
        ->      observable<typename rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>::value_type,    rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>> {
        return  observable<typename rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>::value_type,    rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>(
                                                                                                                                                rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), identity_one_worker(rxsc::make_current_thread())));
    }

    /// concat_map ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable use the CollectionSelector to select an observable and subscribe to that observable.
    /// for each item from all of the selected observables use the ResultSelector to select a value to emit from the new observable that is returned.
    ///
    template<class CollectionSelector, class ResultSelector, class Coordination>
    auto concat_map(CollectionSelector&& s, ResultSelector&& rs, Coordination&& sf) const
        ->      observable<typename rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>::value_type,   rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>> {
        return  observable<typename rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>::value_type,   rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>>(
                                                                                                                                        rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), std::forward<Coordination>(sf)));
    }

    template<class Coordination, class Selector, class... ObservableN>
    struct delayed_combine_latest
    {
        typedef observable<typename rxo::detail::combine_latest<Coordination, Selector, ObservableN...>::value_type, rxo::detail::combine_latest<Coordination, Selector, ObservableN...>> type;
    };

    /// combine_latest ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from all of the observables use the Selector to select a value to emit from the new observable that is returned.
    ///
    template<class Selector, class... ObservableN>
    auto combine_latest(Selector&& s, ObservableN... on) const
        -> typename std::enable_if<!is_coordination<Selector>::value && !is_observable<Selector>::value && rxu::all_true<is_observable<ObservableN>::value...>::value,
                delayed_combine_latest<identity_one_worker, Selector, this_type, ObservableN...>>::type::type {
        return  observable<typename rxo::detail::combine_latest<identity_one_worker, Selector, this_type, ObservableN...>::value_type,  rxo::detail::combine_latest<identity_one_worker, Selector, this_type, ObservableN...>>(
                                                                                                                                        rxo::detail::combine_latest<identity_one_worker, Selector, this_type, ObservableN...>(identity_one_worker(rxsc::make_current_thread()), std::forward<Selector>(s), std::make_tuple(*this, on...)));
    }

    /// combine_latest ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from all of the observables use the Selector to select a value to emit from the new observable that is returned.
    ///
    template<class Coordination, class Selector, class... ObservableN>
    auto combine_latest(Coordination cn, Selector&& s, ObservableN... on) const
        -> typename std::enable_if<is_coordination<Coordination>::value && !is_observable<Selector>::value && rxu::all_true<is_observable<ObservableN>::value...>::value,
                delayed_combine_latest<Coordination, Selector, this_type, ObservableN...>>::type::type {
        return  observable<typename rxo::detail::combine_latest<Coordination, Selector, this_type, ObservableN...>::value_type, rxo::detail::combine_latest<Coordination, Selector, this_type, ObservableN...>>(
                                                                                                                                rxo::detail::combine_latest<Coordination, Selector, this_type, ObservableN...>(std::move(cn), std::forward<Selector>(s), std::make_tuple(*this, on...)));
    }

    /// combine_latest ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from all of the observables use the Selector to select a value to emit from the new observable that is returned.
    ///
    template<class... ObservableN>
    auto combine_latest(ObservableN... on) const
        -> typename std::enable_if<rxu::all_true<is_observable<ObservableN>::value...>::value,
                delayed_combine_latest<identity_one_worker, rxu::detail::pack, this_type, ObservableN...>>::type::type {
        return  observable<typename rxo::detail::combine_latest<identity_one_worker, rxu::detail::pack, this_type, ObservableN...>::value_type, rxo::detail::combine_latest<identity_one_worker, rxu::detail::pack, this_type, ObservableN...>>(
                                                                                                                                                rxo::detail::combine_latest<identity_one_worker, rxu::detail::pack, this_type, ObservableN...>(identity_one_worker(rxsc::make_current_thread()), rxu::pack(), std::make_tuple(*this, on...)));
    }

    /// combine_latest ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from all of the observables use the Selector to select a value to emit from the new observable that is returned.
    ///
    template<class Coordination, class... ObservableN>
    auto combine_latest(Coordination cn, ObservableN... on) const
        -> typename std::enable_if<is_coordination<Coordination>::value && rxu::all_true<is_observable<ObservableN>::value...>::value,
                delayed_combine_latest<Coordination, rxu::detail::pack, this_type, ObservableN...>>::type::type {
        return  observable<typename rxo::detail::combine_latest<Coordination, rxu::detail::pack, this_type, ObservableN...>::value_type,    rxo::detail::combine_latest<Coordination, rxu::detail::pack, this_type, ObservableN...>>(
                                                                                                                                            rxo::detail::combine_latest<Coordination, rxu::detail::pack, this_type, ObservableN...>(std::move(cn), rxu::pack(), std::make_tuple(*this, on...)));
    }

    /// multicast ->
    /// allows connections to the source to be independent of subscriptions
    ///
    template<class Subject>
    auto multicast(Subject sub) const
        ->      connectable_observable<T,   rxo::detail::multicast<T, this_type, Subject>> {
        return  connectable_observable<T,   rxo::detail::multicast<T, this_type, Subject>>(
                                            rxo::detail::multicast<T, this_type, Subject>(*this, std::move(sub)));
    }

    /// synchronize ->
    /// turns a cold observable hot and allows connections to the source to be independent of subscriptions
    ///
    template<class Coordination>
    auto synchronize(Coordination cn, composite_subscription cs = composite_subscription()) const
        -> decltype(EXPLICIT_THIS multicast(rxsub::synchronize<T, Coordination>(std::move(cn), cs))) {
        return                    multicast(rxsub::synchronize<T, Coordination>(std::move(cn), cs));
    }

    /// publish ->
    /// turns a cold observable hot and allows connections to the source to be independent of subscriptions
    ///
    auto publish(composite_subscription cs = composite_subscription()) const
        -> decltype(EXPLICIT_THIS multicast(rxsub::subject<T>(cs))) {
        return                    multicast(rxsub::subject<T>(cs));
    }

    /// scan ->
    /// for each item from this observable use Accumulator to combine items into a value that will be emitted from the new observable that is returned.
    ///
    template<class Seed, class Accumulator>
    auto scan(Seed seed, Accumulator&& a) const
        ->      observable<Seed,    rxo::detail::scan<T, this_type, Accumulator, Seed>> {
        return  observable<Seed,    rxo::detail::scan<T, this_type, Accumulator, Seed>>(
                                    rxo::detail::scan<T, this_type, Accumulator, Seed>(*this, std::forward<Accumulator>(a), seed));
    }

    /// take ->
    /// for the first count items from this observable emit them from the new observable that is returned.
    ///
    ///
    template<class Count>
    auto take(Count t) const
        ->      observable<T,   rxo::detail::take<T, this_type, Count>> {
        return  observable<T,   rxo::detail::take<T, this_type, Count>>(
                                rxo::detail::take<T, this_type, Count>(*this, t));
    }

    /// take_until ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable until on_next occurs on the TriggerSource, emit them from the new observable that is returned.
    ///
    ///
    template<class TriggerSource>
    auto take_until(TriggerSource&& t) const
        -> typename std::enable_if<is_observable<TriggerSource>::value,
                observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>>>::type {
        return  observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>>(
                                rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>(*this, std::forward<TriggerSource>(t), identity_one_worker(rxsc::make_current_thread())));
    }

    /// take_until ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable until on_next occurs on the TriggerSource, emit them from the new observable that is returned.
    ///
    ///
    template<class TriggerSource, class Coordination>
    auto take_until(TriggerSource&& t, Coordination&& sf) const
        -> typename std::enable_if<is_observable<TriggerSource>::value && is_coordination<Coordination>::value,
                observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, Coordination>>>::type {
        return  observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, Coordination>>(
                                rxo::detail::take_until<T, this_type, TriggerSource, Coordination>(*this, std::forward<TriggerSource>(t), std::forward<Coordination>(sf)));
    }
};

template<class T, class SourceOperator>
inline bool operator==(const observable<T, SourceOperator>& lhs, const observable<T, SourceOperator>& rhs) {
    return lhs.source_operator == rhs.source_operator;
}
template<class T, class SourceOperator>
inline bool operator!=(const observable<T, SourceOperator>& lhs, const observable<T, SourceOperator>& rhs) {
    return !(lhs == rhs);
}

// observable<> has static methods to construct observable sources and adaptors.
// observable<> is not constructable
template<>
class observable<void, void>
{
    ~observable();
public:
    template<class T>
    static auto range(T first = 0, T last = std::numeric_limits<T>::max(), ptrdiff_t step = 1)
        -> decltype(rxs::range<T>(first, last, step, identity_one_worker(rxsc::make_current_thread()))) {
        return      rxs::range<T>(first, last, step, identity_one_worker(rxsc::make_current_thread()));
    }
    template<class T, class Coordination>
    static auto range(T first, T last, ptrdiff_t step, Coordination cn)
        -> decltype(rxs::range<T>(first, last, step, std::move(cn))) {
        return      rxs::range<T>(first, last, step, std::move(cn));
    }
    template<class T, class Coordination>
    static auto range(T first, T last, Coordination cn)
        -> decltype(rxs::range<T>(first, last, std::move(cn))) {
        return      rxs::range<T>(first, last, std::move(cn));
    }
    template<class T, class Coordination>
    static auto range(T first, Coordination cn)
        -> decltype(rxs::range<T>(first, std::move(cn))) {
        return      rxs::range<T>(first, std::move(cn));
    }
    template<class T>
    static auto never()
        -> decltype(rxs::never<T>()) {
        return      rxs::never<T>();
    }
    template<class ObservableFactory>
    static auto defer(ObservableFactory of)
        -> decltype(rxs::defer(std::move(of))) {
        return      rxs::defer(std::move(of));
    }
    static auto interval(rxsc::scheduler::clock_type::time_point initial, rxsc::scheduler::clock_type::duration period)
        -> decltype(rxs::interval(initial, period)) {
        return      rxs::interval(initial, period);
    }
    template<class Coordination>
    static auto interval(rxsc::scheduler::clock_type::time_point initial, rxsc::scheduler::clock_type::duration period, Coordination cn)
        -> decltype(rxs::interval(initial, period, std::move(cn))) {
        return      rxs::interval(initial, period, std::move(cn));
    }
    template<class Collection>
    static auto iterate(Collection c)
        -> decltype(rxs::iterate(std::move(c), identity_one_worker(rxsc::make_current_thread()))) {
        return      rxs::iterate(std::move(c), identity_one_worker(rxsc::make_current_thread()));
    }
    template<class Collection, class Coordination>
    static auto iterate(Collection c, Coordination cn)
        -> decltype(rxs::iterate(std::move(c), std::move(cn))) {
        return      rxs::iterate(std::move(c), std::move(cn));
    }
    template<class T>
    static auto from()
        -> decltype(    rxs::from<T>()) {
        return          rxs::from<T>();
    }
    template<class T, class Coordination>
    static auto from(Coordination cn)
        -> typename std::enable_if<is_coordination<Coordination>::value,
            decltype(   rxs::from<T>(std::move(cn)))>::type {
        return          rxs::from<T>(std::move(cn));
    }
    template<class Value0, class... ValueN>
    static auto from(Value0 v0, ValueN... vn)
        -> typename std::enable_if<!is_coordination<Value0>::value,
            decltype(   rxs::from(v0, vn...))>::type {
        return          rxs::from(v0, vn...);
    }
    template<class Coordination, class Value0, class... ValueN>
    static auto from(Coordination cn, Value0 v0, ValueN... vn)
        -> typename std::enable_if<is_coordination<Coordination>::value,
            decltype(   rxs::from(std::move(cn), v0, vn...))>::type {
        return          rxs::from(std::move(cn), v0, vn...);
    }
    template<class T>
    static auto empty()
        -> decltype(from<T>()) {
        return      from<T>();
    }
    template<class T, class Coordination>
    static auto empty(Coordination cn)
        -> decltype(from(std::move(cn), std::array<T, 0>())) {
        return      from(std::move(cn), std::array<T, 0>());
    }
    template<class T>
    static auto just(T v)
        -> decltype(from(std::move(v))) {
        return      from(std::move(v));
    }
    template<class T, class Coordination>
    static auto just(T v, Coordination cn)
        -> decltype(from(std::move(cn), std::move(v))) {
        return      from(std::move(cn), std::move(v));
    }
    template<class T, class Exception>
    static auto error(Exception&& e)
        -> decltype(rxs::error<T>(std::forward<Exception>(e))) {
        return      rxs::error<T>(std::forward<Exception>(e));
    }
    template<class T, class Exception, class Coordination>
    static auto error(Exception&& e, Coordination cn)
        -> decltype(rxs::error<T>(std::forward<Exception>(e), std::move(cn))) {
        return      rxs::error<T>(std::forward<Exception>(e), std::move(cn));
    }
};


}

//
// support range() >> filter() >> subscribe() syntax
// '>>' is spelled 'stream'
//
template<class T, class SourceOperator, class OperatorFactory>
auto operator >> (const rxcpp::observable<T, SourceOperator>& source, OperatorFactory&& of)
    -> decltype(rxcpp::detail::select_chain<T, rxcpp::observable<T, SourceOperator>, OperatorFactory>::type::chain(source, std::forward<OperatorFactory>(of))) {
    return      rxcpp::detail::select_chain<T, rxcpp::observable<T, SourceOperator>, OperatorFactory>::type::chain(source, std::forward<OperatorFactory>(of));
}

//
// support range() | filter() | subscribe() syntax
// '|' is spelled 'pipe'
//
template<class T, class SourceOperator, class OperatorFactory>
auto operator | (const rxcpp::observable<T, SourceOperator>& source, OperatorFactory&& of)
    -> decltype(rxcpp::detail::select_chain<T, rxcpp::observable<T, SourceOperator>, OperatorFactory>::type::chain(source, std::forward<OperatorFactory>(of))) {
    return      rxcpp::detail::select_chain<T, rxcpp::observable<T, SourceOperator>, OperatorFactory>::type::chain(source, std::forward<OperatorFactory>(of));
}

#endif
