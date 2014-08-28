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
    typename std::enable_if<is_subscriber<Subscriber>::value, void>::type
    on_subscribe(Subscriber o) const {
        state->on_subscribe(o.as_dynamic());
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

namespace detail {
template<bool Selector, class Default, class SO>
struct resolve_observable;

template<class Default, class SO>
struct resolve_observable<true, Default, SO>
{
    typedef typename SO::type type;
    typedef typename type::value_type value_type;
    static const bool value = true;
    typedef observable<value_type, type> observable_type;
    template<class... AN>
    static observable_type make(const Default&, AN&&... an) {
        return observable_type(type(std::forward<AN>(an)...));
    }
};
template<class Default, class SO>
struct resolve_observable<false, Default, SO>
{
    static const bool value = false;
    typedef Default observable_type;
    template<class... AN>
    static observable_type make(const observable_type& that, const AN&...) {
        return that;
    }
};
template<class SO>
struct resolve_observable<true, void, SO>
{
    typedef typename SO::type type;
    typedef typename type::value_type value_type;
    static const bool value = true;
    typedef observable<value_type, type> observable_type;
    template<class... AN>
    static observable_type make(AN&&... an) {
        return observable_type(type(std::forward<AN>(an)...));
    }
};
template<class SO>
struct resolve_observable<false, void, SO>
{
    static const bool value = false;
    typedef void observable_type;
    template<class... AN>
    static observable_type make(const AN&...) {
    }
};

}

template<class Selector, class Default, template<class... TN> class SO, class... AN>
struct defer_observable
    : public detail::resolve_observable<Selector::value, Default, rxu::defer_type<SO, AN...>>
{
};

template<class T, class Observable>
class blocking_observable
{
    template<class Obsvbl, class... ArgN>
    static auto blocking_subscribe(const Obsvbl& source, ArgN&&... an)
        -> composite_subscription {
        std::mutex lock;
        std::condition_variable wake;
        composite_subscription cs;
        struct tracking
        {
            ~tracking()
            {
                if (!disposed || !wakened) abort();
            }
            tracking()
            {
                disposed = false;
                wakened = false;
                false_wakes = 0;
                true_wakes = 0;
            }
            std::atomic_bool disposed;
            std::atomic_bool wakened;
            std::atomic_int false_wakes;
            std::atomic_int true_wakes;
        };
        auto track = std::make_shared<tracking>();

        auto scbr = make_subscriber<T>(std::forward<ArgN>(an)...);
        cs = scbr.get_subscription();
        cs.add(
            [&, track](){
                // OSX geting invalid x86 op if notify_one is after the disposed = true
                // presumably because the condition_variable may already have been awakened
                // and is now sitting in a while loop on disposed
                wake.notify_one();
                track->disposed = true;
            });
        std::unique_lock<std::mutex> guard(lock);
        source.subscribe(std::move(scbr));
        wake.wait(guard,
            [&, track](){
                // this is really not good.
                // false wakeups were never followed by true wakeups so..

                // anyways this gets triggered before disposed is set now so wait.
                while (!track->disposed) {
                    ++track->false_wakes;
                }
                ++track->true_wakes;
                return true;
            });
        track->wakened = true;
        if (!track->disposed || !track->wakened) abort();
        return composite_subscription::empty();
    }

public:
    typedef typename std::decay<Observable>::type observable_type;
    observable_type source;
    ~blocking_observable()
    {
    }
    blocking_observable(observable_type s) : source(std::move(s)) {}

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
    template<class... ArgN>
    auto subscribe(ArgN&&... an) const
        -> composite_subscription {
        return blocking_subscribe(source, std::forward<ArgN>(an)...);
    }

    T first() {
        rxu::maybe<T> result;
        composite_subscription cs;
        subscribe(cs, [&](T v){result.reset(v); cs.unsubscribe();});
        if (result.empty()) abort();
        return result.get();
    }

    T last() const {
        rxu::maybe<T> result;
        subscribe([&](T v){result.reset(v);});
        if (result.empty()) abort();
        return result.get();
    }

    int count() const {
        return source.count().as_blocking().last();
    }
    T sum() const {
        return source.sum().as_blocking().last();
    }
    double average() const {
        return source.average().as_blocking().last();
    }
};

template<>
class observable<void, void>;


template<class T, class SourceOperator>
class observable
    : public observable_base<T>
{
    static_assert(std::is_same<T, typename SourceOperator::value_type>::value, "SourceOperator::value_type must be the same as T in observable<T, SourceOperator>");

    typedef observable<T, SourceOperator> this_type;

public:
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

        static_assert(is_subscriber<subscriber_type>::value, "subscribe must be passed a subscriber");
        static_assert(std::is_same<typename source_operator_type::value_type, T>::value && std::is_convertible<T*, typename subscriber_type::value_type*>::value, "the value types in the sequence must match or be convertible");
        static_assert(detail::has_on_subscribe_for<subscriber_type, source_operator_type>::value, "inner must have on_subscribe method that accepts this subscriber ");

        trace_activity().subscribe_enter(*this, o);

        if (!o.is_subscribed()) {
            trace_activity().subscribe_return(*this);
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

        trace_activity().subscribe_return(*this);
        return o.get_subscription();
    }

public:
    typedef T value_type;

    static_assert(rxo::is_operator<source_operator_type>::value || rxs::is_source<source_operator_type>::value, "observable must wrap an operator or source");

    ~observable()
    {
    }

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
    /// returns a new observable that performs type-forgetting conversion of this observable
    ///
    observable<T> as_dynamic() const {
        return *this;
    }

    ///
    /// returns new observable that contains the blocking methods for this observable
    ///
    blocking_observable<T, this_type> as_blocking() const {
        return blocking_observable<T, this_type>(*this);
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
    template<class ResultType, class Operator>
    auto lift(Operator&& op) const
        ->      observable<typename rxo::detail::lift_operator<ResultType, source_operator_type, Operator>::value_type, rxo::detail::lift_operator<ResultType, source_operator_type, Operator>> {
        return  observable<typename rxo::detail::lift_operator<ResultType, source_operator_type, Operator>::value_type, rxo::detail::lift_operator<ResultType, source_operator_type, Operator>>(
                                                                                                                        rxo::detail::lift_operator<ResultType, source_operator_type, Operator>(source_operator, std::forward<Operator>(op)));
        static_assert(detail::is_lift_function_for<T, subscriber<ResultType>, Operator>::value, "Function passed for lift() must have the signature subscriber<...>(subscriber<T, ...>)");
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
    template<class... ArgN>
    auto subscribe(ArgN&&... an) const
        -> composite_subscription {
        return detail_subscribe(make_subscriber<T>(std::forward<ArgN>(an)...));
    }

    /// filter (AKA Where) ->
    /// for each item from this observable use Predicate to select which items to emit from the new observable that is returned.
    ///
    template<class Predicate>
    auto filter(Predicate p) const
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::filter<T, Predicate>(std::move(p)))) {
        return                    lift<T>(rxo::detail::filter<T, Predicate>(std::move(p)));
    }

    /// finally () ->
    ///
    template<class LastCall>
    auto finally(LastCall lc) const
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::finally<T, LastCall>(std::move(lc)))) {
        return                    lift<T>(rxo::detail::finally<T, LastCall>(std::move(lc)));
    }

    /// map (AKA Select) ->
    /// for each item from this observable use Selector to produce an item to emit from the new observable that is returned.
    ///
    template<class Selector>
    auto map(Selector s) const
        -> decltype(EXPLICIT_THIS lift<typename rxo::detail::map<T, Selector>::value_type>(rxo::detail::map<T, Selector>(std::move(s)))) {
        return                    lift<typename rxo::detail::map<T, Selector>::value_type>(rxo::detail::map<T, Selector>(std::move(s)));
    }

    /// distinct_until_changed ->
    /// for each item from this observable, filter out repeated values and emit only changes from the new observable that is returned.
    ///
    auto distinct_until_changed() const
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::distinct_until_changed<T>())) {
        return                    lift<T>(rxo::detail::distinct_until_changed<T>());
    }

    /// window ->
    /// produce observables containing count items emitted by this observable
    ///
    auto window(int count) const
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window<T>(count, count))) {
        return                    lift<observable<T>>(rxo::detail::window<T>(count, count));
    }

    /// window ->
    /// produce observables containing count items emitted by this observable
    ///
    auto window(int count, int skip) const
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window<T>(count, skip))) {
        return                    lift<observable<T>>(rxo::detail::window<T>(count, skip));
    }

    /// buffer ->
    /// collect count items from this observable and produce a vector of them to emit from the new observable that is returned.
    ///
    auto buffer(int count) const
        -> decltype(EXPLICIT_THIS lift<std::vector<T>>(rxo::detail::buffer_count<T>(count, count))) {
        return                    lift<std::vector<T>>(rxo::detail::buffer_count<T>(count, count));
    }

    /// buffer ->
    /// start a new vector every skip items and collect count items from this observable into each vector to emit from the new observable that is returned.
    ///
    auto buffer(int count, int skip) const
        -> decltype(EXPLICIT_THIS lift<std::vector<T>>(rxo::detail::buffer_count<T>(count, skip))) {
        return                    lift<std::vector<T>>(rxo::detail::buffer_count<T>(count, skip));
    }

    template<class Coordination>
    struct defer_switch_on_next : public defer_observable<
        is_observable<value_type>,
        this_type,
        rxo::detail::switch_on_next, value_type, observable<value_type>, Coordination>
    {
    };

    /// switch_on_next (AKA Switch) ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe to that observable after unsubscribing from any previous subscription.
    ///
    auto switch_on_next() const
        -> typename defer_switch_on_next<identity_one_worker>::observable_type {
        return      defer_switch_on_next<identity_one_worker>::make(*this, *this, identity_current_thread());
    }

    /// switch_on_next (AKA Switch) ->
    /// The coodination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe to that observable after unsubscribing from any previous subscription.
    ///
    template<class Coordination>
    auto switch_on_next(Coordination cn) const
        ->  typename std::enable_if<
                        defer_switch_on_next<Coordination>::value,
            typename    defer_switch_on_next<Coordination>::observable_type>::type {
        return          defer_switch_on_next<Coordination>::make(*this, *this, std::move(cn));
    }

    template<class Coordination>
    struct defer_merge : public defer_observable<
        is_observable<value_type>,
        this_type,
        rxo::detail::merge, value_type, observable<value_type>, Coordination>
    {
    };

    /// merge ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    auto merge() const
        -> typename defer_merge<identity_one_worker>::observable_type {
        return      defer_merge<identity_one_worker>::make(*this, *this, identity_current_thread());
    }

    /// merge ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Coordination>
    auto merge(Coordination cn) const
        ->  typename std::enable_if<
                        defer_merge<Coordination>::value,
            typename    defer_merge<Coordination>::observable_type>::type {
        return          defer_merge<Coordination>::make(*this, *this, std::move(cn));
    }

    template<class Coordination, class Value0>
    struct defer_merge_from : public defer_observable<
        rxu::all_true<
            is_coordination<Coordination>::value,
            is_observable<Value0>::value>,
        this_type,
        rxo::detail::merge, observable<value_type>, observable<observable<value_type>>, Coordination>
    {
    };

    /// merge ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Value0, class... ValueN>
    auto merge(Value0 v0, ValueN... vn) const
        ->  typename std::enable_if<
                        defer_merge_from<identity_one_worker, Value0>::value,
            typename    defer_merge_from<identity_one_worker, Value0>::observable_type>::type {
        return          defer_merge_from<identity_one_worker, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), identity_current_thread());
    }

    /// merge ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Coordination, class Value0, class... ValueN>
    auto merge(Coordination cn, Value0 v0, ValueN... vn) const
        ->  typename std::enable_if<
                        defer_merge_from<Coordination, Value0>::value,
            typename    defer_merge_from<Coordination, Value0>::observable_type>::type {
        return          defer_merge_from<Coordination, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), std::move(cn));
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
                                                                                                                                            rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), identity_current_thread()));
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

    template<class Coordination>
    struct defer_concat : public defer_observable<
        is_observable<value_type>,
        this_type,
        rxo::detail::concat, value_type, observable<value_type>, Coordination>
    {
    };

    /// concat ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe to one at a time. in the order received.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    auto concat() const
        -> typename defer_concat<identity_one_worker>::observable_type {
        return      defer_concat<identity_one_worker>::make(*this, *this, identity_current_thread());
    }

    /// concat ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe to one at a time. in the order received.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Coordination>
    auto concat(Coordination cn) const
        ->  typename std::enable_if<
                        defer_concat<Coordination>::value,
            typename    defer_concat<Coordination>::observable_type>::type {
        return          defer_concat<Coordination>::make(*this, *this, std::move(cn));
    }

    template<class Coordination, class Value0>
    struct defer_concat_from : public defer_observable<
        rxu::all_true<
            is_coordination<Coordination>::value,
            is_observable<Value0>::value>,
        this_type,
        rxo::detail::concat, observable<value_type>, observable<observable<value_type>>, Coordination>
    {
    };

    /// concat ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable subscribe to one at a time. in the order received.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Value0, class... ValueN>
    auto concat(Value0 v0, ValueN... vn) const
        ->  typename std::enable_if<
                        defer_concat_from<identity_one_worker, Value0>::value,
            typename    defer_concat_from<identity_one_worker, Value0>::observable_type>::type {
        return          defer_concat_from<identity_one_worker, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), identity_current_thread());
    }

    /// concat ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable subscribe to one at a time. in the order received.
    /// for each item from all of the nested observables deliver from the new observable that is returned.
    ///
    template<class Coordination, class Value0, class... ValueN>
    auto concat(Coordination cn, Value0 v0, ValueN... vn) const
        ->  typename std::enable_if<
                        defer_concat_from<Coordination, Value0>::value,
            typename    defer_concat_from<Coordination, Value0>::observable_type>::type {
        return          defer_concat_from<Coordination, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), std::move(cn));
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
                                                                                                                                                rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), identity_current_thread()));
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
    struct defer_combine_latest : public defer_observable<
        rxu::all_true<is_coordination<Coordination>::value, !is_coordination<Selector>::value, !is_observable<Selector>::value, is_observable<ObservableN>::value...>,
        this_type,
        rxo::detail::combine_latest, Coordination, Selector, ObservableN...>
    {
    };

    /// combine_latest ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from all of the observables use the Selector to select a value to emit from the new observable that is returned.
    ///
    template<class Selector, class... ObservableN>
    auto combine_latest(Selector&& s, ObservableN... on) const
        ->  typename std::enable_if<
                        defer_combine_latest<identity_one_worker, Selector, this_type, ObservableN...>::value,
            typename    defer_combine_latest<identity_one_worker, Selector, this_type, ObservableN...>::observable_type>::type {
        return          defer_combine_latest<identity_one_worker, Selector, this_type, ObservableN...>::make(*this, identity_current_thread(), std::forward<Selector>(s), std::make_tuple(*this, on...));
    }

    /// combine_latest ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from all of the observables use the Selector to select a value to emit from the new observable that is returned.
    ///
    template<class Coordination, class Selector, class... ObservableN>
    auto combine_latest(Coordination cn, Selector&& s, ObservableN... on) const
        ->  typename std::enable_if<
                        defer_combine_latest<Coordination, Selector, this_type, ObservableN...>::value,
            typename    defer_combine_latest<Coordination, Selector, this_type, ObservableN...>::observable_type>::type {
        return          defer_combine_latest<Coordination, Selector, this_type, ObservableN...>::make(*this, std::move(cn), std::forward<Selector>(s), std::make_tuple(*this, on...));
    }

    /// combine_latest ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from all of the observables use the Selector to select a value to emit from the new observable that is returned.
    ///
    template<class... ObservableN>
    auto combine_latest(ObservableN... on) const
        ->  typename std::enable_if<
                        defer_combine_latest<identity_one_worker, rxu::detail::pack, this_type, ObservableN...>::value,
            typename    defer_combine_latest<identity_one_worker, rxu::detail::pack, this_type, ObservableN...>::observable_type>::type {
        return          defer_combine_latest<identity_one_worker, rxu::detail::pack, this_type, ObservableN...>::make(*this, identity_current_thread(), rxu::pack(), std::make_tuple(*this, on...));
    }

    /// combine_latest ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from all of the observables use the Selector to select a value to emit from the new observable that is returned.
    ///
    template<class Coordination, class... ObservableN>
    auto combine_latest(Coordination cn, ObservableN... on) const
        ->  typename std::enable_if<
                        defer_combine_latest<Coordination, rxu::detail::pack, this_type, ObservableN...>::value,
            typename    defer_combine_latest<Coordination, rxu::detail::pack, this_type, ObservableN...>::observable_type>::type {
        return          defer_combine_latest<Coordination, rxu::detail::pack, this_type, ObservableN...>::make(*this, std::move(cn), rxu::pack(), std::make_tuple(*this, on...));
    }

    /// group_by ->
    ///
    template<class KeySelector, class MarbleSelector, class BinaryPredicate>
    inline auto group_by(KeySelector ks, MarbleSelector ms, BinaryPredicate p) const
        -> decltype(EXPLICIT_THIS lift<typename rxo::detail::group_by_traits<T, this_type, KeySelector, MarbleSelector, BinaryPredicate>::grouped_observable_type>(rxo::detail::group_by<T, this_type, KeySelector, MarbleSelector, BinaryPredicate>(std::move(ks), std::move(ms), std::move(p)))) {
        return                    lift<typename rxo::detail::group_by_traits<T, this_type, KeySelector, MarbleSelector, BinaryPredicate>::grouped_observable_type>(rxo::detail::group_by<T, this_type, KeySelector, MarbleSelector, BinaryPredicate>(std::move(ks), std::move(ms), std::move(p)));
    }

    /// group_by ->
    ///
    template<class KeySelector, class MarbleSelector>
    inline auto group_by(KeySelector ks, MarbleSelector ms) const
        -> decltype(EXPLICIT_THIS lift<typename rxo::detail::group_by_traits<T, this_type, KeySelector, MarbleSelector, rxu::less>::grouped_observable_type>(rxo::detail::group_by<T, this_type, KeySelector, MarbleSelector, rxu::less>(std::move(ks), std::move(ms), rxu::less()))) {
        return                    lift<typename rxo::detail::group_by_traits<T, this_type, KeySelector, MarbleSelector, rxu::less>::grouped_observable_type>(rxo::detail::group_by<T, this_type, KeySelector, MarbleSelector, rxu::less>(std::move(ks), std::move(ms), rxu::less()));
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

    /// publish_synchronized ->
    /// turns a cold observable hot and allows connections to the source to be independent of subscriptions.
    /// all values are queued and delivered using the scheduler from the supplied coordination
    ///
    template<class Coordination>
    auto publish_synchronized(Coordination cn, composite_subscription cs = composite_subscription()) const
        -> decltype(EXPLICIT_THIS multicast(rxsub::synchronize<T, Coordination>(std::move(cn), cs))) {
        return                    multicast(rxsub::synchronize<T, Coordination>(std::move(cn), cs));
    }

    /// publish ->
    /// turns a cold observable hot and allows connections to the source to be independent of subscriptions
    /// NOTE: multicast of a subject
    ///
    auto publish(composite_subscription cs = composite_subscription()) const
        -> decltype(EXPLICIT_THIS multicast(rxsub::subject<T>(cs))) {
        return                    multicast(rxsub::subject<T>(cs));
    }

    /// publish ->
    /// turns a cold observable hot, sends the most recent value to any new subscriber and allows connections to the source to be independent of subscriptions
    /// NOTE: multicast of a behavior
    ///
    auto publish(T first, composite_subscription cs = composite_subscription()) const
        -> decltype(EXPLICIT_THIS multicast(rxsub::behavior<T>(first, cs))) {
        return      multicast(rxsub::behavior<T>(first, cs));
    }

    /// subscribe_on ->
    /// subscription and unsubscription are queued and delivered using the scheduler from the supplied coordination
    ///
    template<class Coordination>
    auto subscribe_on(Coordination cn) const
        ->      observable<typename rxo::detail::subscribe_on<T, this_type, Coordination>::value_type,  rxo::detail::subscribe_on<T, this_type, Coordination>> {
        return  observable<typename rxo::detail::subscribe_on<T, this_type, Coordination>::value_type,  rxo::detail::subscribe_on<T, this_type, Coordination>>(
                                                                                                        rxo::detail::subscribe_on<T, this_type, Coordination>(*this, std::move(cn)));
    }

    /// observe_on ->
    /// all values are queued and delivered using the scheduler from the supplied coordination
    ///
    template<class Coordination>
    auto observe_on(Coordination cn) const
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::observe_on<T, Coordination>(std::move(cn)))) {
        return                    lift<T>(rxo::detail::observe_on<T, Coordination>(std::move(cn)));
    }

    /// reduce ->
    /// for each item from this observable use Accumulator to combine items, when completed use ResultSelector to produce a value that will be emitted from the new observable that is returned.
    ///
    template<class Seed, class Accumulator, class ResultSelector>
    auto reduce(Seed seed, Accumulator&& a, ResultSelector&& rs) const
        ->      observable<typename rxo::detail::reduce<T, source_operator_type, Accumulator, ResultSelector, Seed>::value_type,    rxo::detail::reduce<T, source_operator_type, Accumulator, ResultSelector, Seed>> {
        return  observable<typename rxo::detail::reduce<T, source_operator_type, Accumulator, ResultSelector, Seed>::value_type,    rxo::detail::reduce<T, source_operator_type, Accumulator, ResultSelector, Seed>>(
                                                                                                                                    rxo::detail::reduce<T, source_operator_type, Accumulator, ResultSelector, Seed>(source_operator, std::forward<Accumulator>(a), std::forward<ResultSelector>(rs), seed));
    }

    template<class Seed, class Accumulator, class ResultSelector>
    struct defer_reduce : public defer_observable<
        rxu::all_true<
            rxu::defer_trait<rxo::detail::is_accumulate_function_for, T, Seed, Accumulator>::value,
            rxu::defer_trait<rxo::detail::is_result_function_for, Seed, ResultSelector>::value>,
        void,
        rxo::detail::reduce, T, source_operator_type, Accumulator, ResultSelector, Seed>
    {
    };

    /// first ->
    /// for each item from this observable reduce it by sending only the first item.
    ///
    auto first() const
        -> observable<T>;

    /// last ->
    /// for each item from this observable reduce it by sending only the last item.
    ///
    auto last() const
        -> observable<T>;

    /// count ->
    /// for each item from this observable reduce it by incrementing a count.
    ///
    auto count() const
        -> observable<int>;

    /// sum ->
    /// for each item from this observable reduce it by adding to the previous items.
    ///
    auto sum() const
        -> typename defer_reduce<rxu::defer_seed_type<rxo::detail::initialize_seeder, T>, rxu::plus, rxu::defer_type<identity_for, T>>::observable_type {
        return      defer_reduce<rxu::defer_seed_type<rxo::detail::initialize_seeder, T>, rxu::plus, rxu::defer_type<identity_for, T>>::make(source_operator, rxu::plus(), identity_for<T>(), rxo::detail::initialize_seeder<T>().seed());
    }

    /// average ->
    /// for each item from this observable reduce it by adding to the previous values and then dividing by the number of items at the end.
    ///
    auto average() const
        -> typename defer_reduce<rxu::defer_seed_type<rxo::detail::average, T>, rxu::defer_type<rxo::detail::average, T>, rxu::defer_type<rxo::detail::average, T>>::observable_type {
        return      defer_reduce<rxu::defer_seed_type<rxo::detail::average, T>, rxu::defer_type<rxo::detail::average, T>, rxu::defer_type<rxo::detail::average, T>>::make(source_operator, rxo::detail::average<T>(), rxo::detail::average<T>(), rxo::detail::average<T>().seed());
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

    /// skip ->
    /// make new observable with skipped first count items from this observable
    ///
    ///
    template<class Count>
    auto skip(Count t) const
        ->      observable<T,   rxo::detail::skip<T, this_type, Count>> {
        return  observable<T,   rxo::detail::skip<T, this_type, Count>>(
                                rxo::detail::skip<T, this_type, Count>(*this, t));
    }

    /// skip_until ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// make new observable with items skipped until on_next occurs on the TriggerSource
    ///
    ///
    template<class TriggerSource>
    auto skip_until(TriggerSource&& t) const
        -> typename std::enable_if<is_observable<TriggerSource>::value,
                observable<T,   rxo::detail::skip_until<T, this_type, TriggerSource, identity_one_worker>>>::type {
        return  observable<T,   rxo::detail::skip_until<T, this_type, TriggerSource, identity_one_worker>>(
                                rxo::detail::skip_until<T, this_type, TriggerSource, identity_one_worker>(*this, std::forward<TriggerSource>(t), identity_one_worker(rxsc::make_current_thread())));
    }

    /// skip_until ->
    /// The coordination is used to synchronize sources from different contexts.
    /// make new observable with items skipped until on_next occurs on the TriggerSource
    ///
    ///
    template<class TriggerSource, class Coordination>
    auto skip_until(TriggerSource&& t, Coordination&& sf) const
        -> typename std::enable_if<is_observable<TriggerSource>::value && is_coordination<Coordination>::value,
                observable<T,   rxo::detail::skip_until<T, this_type, TriggerSource, Coordination>>>::type {
        return  observable<T,   rxo::detail::skip_until<T, this_type, TriggerSource, Coordination>>(
                                rxo::detail::skip_until<T, this_type, TriggerSource, Coordination>(*this, std::forward<TriggerSource>(t), std::forward<Coordination>(sf)));
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
    auto take_until(TriggerSource t) const
        -> typename std::enable_if<is_observable<TriggerSource>::value,
                observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>>>::type {
        return  observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>>(
                                rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>(*this, std::move(t), identity_current_thread()));
    }

    /// take_until ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable until on_next occurs on the TriggerSource, emit them from the new observable that is returned.
    ///
    ///
    template<class TriggerSource, class Coordination>
    auto take_until(TriggerSource t, Coordination sf) const
        -> typename std::enable_if<is_observable<TriggerSource>::value && is_coordination<Coordination>::value,
                observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, Coordination>>>::type {
        return  observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, Coordination>>(
                                rxo::detail::take_until<T, this_type, TriggerSource, Coordination>(*this, std::move(t), std::move(sf)));
    }

    /// take_until ->
    /// All sources must be synchronized! This means that calls across all the subscribers must be serial.
    /// for each item from this observable until the specified time, emit them from the new observable that is returned.
    ///
    ///
    template<class TimePoint>
    auto take_until(TimePoint when) const
        -> typename std::enable_if<std::is_convertible<TimePoint, rxsc::scheduler::clock_type::time_point>::value,
                observable<T,   rxo::detail::take_until<T, this_type, decltype(rxs::interval(when, identity_current_thread())), identity_one_worker>>>::type {
        auto cn = identity_current_thread();
        return  take_until(rxs::interval(when, cn), cn);
    }

    /// take_until ->
    /// The coordination is used to synchronize sources from different contexts.
    /// for each item from this observable until the specified time, emit them from the new observable that is returned.
    ///
    ///
    template<class Coordination>
    auto take_until(rxsc::scheduler::clock_type::time_point when, Coordination cn) const
        -> typename std::enable_if<is_coordination<Coordination>::value,
                observable<T,   rxo::detail::take_until<T, this_type, decltype(rxs::interval(when, cn)), Coordination>>>::type {
        return  take_until(rxs::interval(when, cn), cn);
    }

    /// repeat ->
    /// infinitely repeats this observable
    ///
    ///
    auto repeat() const
        ->      observable<T, rxo::detail::repeat<T, this_type, int>> {
        return  observable<T, rxo::detail::repeat<T, this_type, int>>(
            rxo::detail::repeat<T, this_type, int>(*this, 0));
    }

    /// repeat ->
    /// repeats this observable for given number of times
    ///
    ///
    template<class Count>
    auto repeat(Count t) const
        ->      observable<T, rxo::detail::repeat<T, this_type, Count>> {
        return  observable<T, rxo::detail::repeat<T, this_type, Count>>(
            rxo::detail::repeat<T, this_type, Count>(*this, t));
    }

    /// retry ->
    /// infinitely retrys this observable
    ///
    ///
    auto retry() const
        ->      observable<T, rxo::detail::retry<T, this_type, int>> {
        return  observable<T, rxo::detail::retry<T, this_type, int>>(
            rxo::detail::retry<T, this_type, int>(*this, 0));
    }

    /// retry ->
    /// retrys this observable for given number of times
    ///
    ///
    template<class Count>
    auto retry(Count t) const
        ->      observable<T, rxo::detail::retry<T, this_type, Count>> {
        return  observable<T, rxo::detail::retry<T, this_type, Count>>(
            rxo::detail::retry<T, this_type, Count>(*this, t));
    }

    /// start_with ->
    /// start with the supplied values, then concatenate this observable
    ///
    ///
    template<class Value0, class... ValueN>
    auto start_with(Value0 v0, ValueN... vn) const
        -> decltype(rxo::start_with(*(this_type*)nullptr, std::move(v0), std::move(vn)...)) {
        return      rxo::start_with(*this, std::move(v0), std::move(vn)...);
    }

};

template<class T, class SourceOperator>
auto observable<T, SourceOperator>::last() const
    -> observable<T> {
    rxu::maybe<T> seed;
    return this->reduce(seed, [](rxu::maybe<T>, T t){return rxu::maybe<T>(std::move(t));}, [](rxu::maybe<T> result){return result.get();});
}

template<class T, class SourceOperator>
auto observable<T, SourceOperator>::first() const
    -> observable<T> {
    return this->take(1).last();
}

template<class T, class SourceOperator>
auto observable<T, SourceOperator>::count() const
    -> observable<int> {
    return this->reduce(0, [](int current, const T&){return ++current;}, [](int result){return result;});
}

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
    template<class T, class OnSubscribe>
    static auto create(OnSubscribe os)
        -> decltype(rxs::create<T>(std::move(os))) {
        return      rxs::create<T>(std::move(os));
    }
    template<class T>
    static auto range(T first = 0, T last = std::numeric_limits<T>::max(), ptrdiff_t step = 1)
        -> decltype(rxs::range<T>(first, last, step, identity_current_thread())) {
        return      rxs::range<T>(first, last, step, identity_current_thread());
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
    static auto interval(rxsc::scheduler::clock_type::time_point when)
        -> decltype(rxs::interval(when)) {
        return      rxs::interval(when);
    }
    template<class Coordination>
    static auto interval(rxsc::scheduler::clock_type::time_point when, Coordination cn)
        -> decltype(rxs::interval(when, std::move(cn))) {
        return      rxs::interval(when, std::move(cn));
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
        -> decltype(rxs::iterate(std::move(c), identity_current_thread())) {
        return      rxs::iterate(std::move(c), identity_current_thread());
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
    template<class Observable, class Value0, class... ValueN>
    static auto start_with(Observable o, Value0 v0, ValueN... vn)
        -> decltype(rxs::from(typename Observable::value_type(v0), typename Observable::value_type(vn)...).concat(o)) {
        return      rxs::from(typename Observable::value_type(v0), typename Observable::value_type(vn)...).concat(o);
    }
    template<class ResourceFactory, class ObservableFactory>
    static auto scope(ResourceFactory rf, ObservableFactory of)
        -> decltype(rxs::scope(std::move(rf), std::move(of))) {
        return      rxs::scope(std::move(rf), std::move(of));
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
