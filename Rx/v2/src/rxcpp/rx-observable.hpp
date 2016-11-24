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

template<class Subscriber, class T>
struct has_on_subscribe_for
{
    struct not_void {};
    template<class CS, class CT>
    static auto check(int) -> decltype((*(CT*)nullptr).on_subscribe(*(CS*)nullptr));
    template<class CS, class CT>
    static not_void check(...);

    typedef decltype(check<rxu::decay_t<Subscriber>, T>(0)) detail_result;
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
        rxu::decay_t<SO> so = std::forward<SO>(source);
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

/*!
    \brief a source of values whose methods block until all values have been emitted. subscribe or use one of the operator methods that reduce the values emitted to a single value.

    \ingroup group-observable

*/
template<class T, class Observable>
class blocking_observable
{
    template<class Obsvbl, class... ArgN>
    static auto blocking_subscribe(const Obsvbl& source, bool do_rethrow, ArgN&&... an)
        -> void {
        std::mutex lock;
        std::condition_variable wake;
        std::exception_ptr error;

        struct tracking
        {
            ~tracking()
            {
                if (!disposed || !wakened) std::terminate();
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

        auto dest = make_subscriber<T>(std::forward<ArgN>(an)...);

        // keep any error to rethrow at the end.
        auto scbr = make_subscriber<T>(
            dest,
            [&](T t){dest.on_next(t);},
            [&](std::exception_ptr e){
                if (do_rethrow) {
                    error = e;
                } else {
                    dest.on_error(e);
                }
            },
            [&](){dest.on_completed();}
            );

        auto cs = scbr.get_subscription();
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
        if (!track->disposed || !track->wakened) std::terminate();

        if (error) {std::rethrow_exception(error);}
    }

public:
    typedef rxu::decay_t<Observable> observable_type;
    observable_type source;
    ~blocking_observable()
    {
    }
    blocking_observable(observable_type s) : source(std::move(s)) {}

    ///
    /// `subscribe` will cause this observable to emit values to the provided subscriber.
    ///
    /// \return void
    ///
    /// \param an... - the arguments are passed to make_subscriber().
    ///
    /// callers must provide enough arguments to make a subscriber.
    /// overrides are supported. thus
    ///   `subscribe(thesubscriber, composite_subscription())`
    /// will take `thesubscriber.get_observer()` and the provided
    /// subscription and subscribe to the new subscriber.
    /// the `on_next`, `on_error`, `on_completed` methods can be supplied instead of an observer
    /// if a subscription or subscriber is not provided then a new subscription will be created.
    ///
    template<class... ArgN>
    auto subscribe(ArgN&&... an) const
        -> void {
        return blocking_subscribe(source, false, std::forward<ArgN>(an)...);
    }

    ///
    /// `subscribe_with_rethrow` will cause this observable to emit values to the provided subscriber.
    ///
    /// \note  If the source observable calls on_error, the raised exception is rethrown by this method.
    ///
    /// \note  If the source observable calls on_error, the `on_error` method on the subscriber will not be called.
    ///
    /// \return void
    ///
    /// \param an... - the arguments are passed to make_subscriber().
    ///
    /// callers must provide enough arguments to make a subscriber.
    /// overrides are supported. thus
    ///   `subscribe(thesubscriber, composite_subscription())`
    /// will take `thesubscriber.get_observer()` and the provided
    /// subscription and subscribe to the new subscriber.
    /// the `on_next`, `on_error`, `on_completed` methods can be supplied instead of an observer
    /// if a subscription or subscriber is not provided then a new subscription will be created.
    ///
    template<class... ArgN>
    auto subscribe_with_rethrow(ArgN&&... an) const
        -> void {
        return blocking_subscribe(source, true, std::forward<ArgN>(an)...);
    }

    /*! Return the first item emitted by this blocking_observable, or throw an std::runtime_error exception if it emits no items.

        \return  The first item emitted by this blocking_observable.

        \note  If the source observable calls on_error, the raised exception is rethrown by this method.

        \sample
        When the source observable emits at least one item:
        \snippet blocking_observable.cpp blocking first sample
        \snippet output.txt blocking first sample

        When the source observable is empty:
        \snippet blocking_observable.cpp blocking first empty sample
        \snippet output.txt blocking first empty sample
    */
    template<class... AN>
    auto first(AN**...) -> delayed_type_t<T, AN...> const {
        rxu::maybe<T> result;
        composite_subscription cs;
        subscribe_with_rethrow(
            cs,
            [&](T v){result.reset(v); cs.unsubscribe();});
        if (result.empty())
            throw rxcpp::empty_error("first() requires a stream with at least one value");
        return result.get();
        static_assert(sizeof...(AN) == 0, "first() was passed too many arguments.");
    }

    /*! Return the last item emitted by this blocking_observable, or throw an std::runtime_error exception if it emits no items.

        \return  The last item emitted by this blocking_observable.

        \note  If the source observable calls on_error, the raised exception is rethrown by this method.

        \sample
        When the source observable emits at least one item:
        \snippet blocking_observable.cpp blocking last sample
        \snippet output.txt blocking last sample

        When the source observable is empty:
        \snippet blocking_observable.cpp blocking last empty sample
        \snippet output.txt blocking last empty sample
    */
    template<class... AN>
    auto last(AN**...) -> delayed_type_t<T, AN...> const {
        rxu::maybe<T> result;
        subscribe_with_rethrow(
            [&](T v){result.reset(v);});
        if (result.empty())
            throw rxcpp::empty_error("last() requires a stream with at least one value");
        return result.get();
        static_assert(sizeof...(AN) == 0, "last() was passed too many arguments.");
    }

    /*! Return the total number of items emitted by this blocking_observable.

        \return  The total number of items emitted by this blocking_observable.

        \sample
        \snippet blocking_observable.cpp blocking count sample
        \snippet output.txt blocking count sample

        When the source observable calls on_error:
        \snippet blocking_observable.cpp blocking count error sample
        \snippet output.txt blocking count error sample
    */
    int count() const {
        int result = 0;
        source.count().as_blocking().subscribe_with_rethrow(
            [&](int v){result = v;});
        return result;
    }

    /*! Return the sum of all items emitted by this blocking_observable, or throw an std::runtime_error exception if it emits no items.

        \return  The sum of all items emitted by this blocking_observable.

        \sample
        When the source observable emits at least one item:
        \snippet blocking_observable.cpp blocking sum sample
        \snippet output.txt blocking sum sample

        When the source observable is empty:
        \snippet blocking_observable.cpp blocking sum empty sample
        \snippet output.txt blocking sum empty sample

        When the source observable calls on_error:
        \snippet blocking_observable.cpp blocking sum error sample
        \snippet output.txt blocking sum error sample
    */
    T sum() const {
        return source.sum().as_blocking().last();
    }

    /*! Return the average value of all items emitted by this blocking_observable, or throw an std::runtime_error exception if it emits no items.

        \return  The average value of all items emitted by this blocking_observable.

        \sample
        When the source observable emits at least one item:
        \snippet blocking_observable.cpp blocking average sample
        \snippet output.txt blocking average sample

        When the source observable is empty:
        \snippet blocking_observable.cpp blocking average empty sample
        \snippet output.txt blocking average empty sample

        When the source observable calls on_error:
        \snippet blocking_observable.cpp blocking average error sample
        \snippet output.txt blocking average error sample
    */
    double average() const {
        return source.average().as_blocking().last();
    }

    /*! Return the max of all items emitted by this blocking_observable, or throw an std::runtime_error exception if it emits no items.

    \return  The max of all items emitted by this blocking_observable.

    \sample
    When the source observable emits at least one item:
    \snippet blocking_observable.cpp blocking max sample
    \snippet output.txt blocking max sample

    When the source observable is empty:
    \snippet blocking_observable.cpp blocking max empty sample
    \snippet output.txt blocking max empty sample

    When the source observable calls on_error:
    \snippet blocking_observable.cpp blocking max error sample
    \snippet output.txt blocking max error sample
*/
    T max() const {
        return source.max().as_blocking().last();
    }

    /*! Return the min of all items emitted by this blocking_observable, or throw an std::runtime_error exception if it emits no items.

    \return  The min of all items emitted by this blocking_observable.

    \sample
    When the source observable emits at least one item:
    \snippet blocking_observable.cpp blocking min sample
    \snippet output.txt blocking min sample

    When the source observable is empty:
    \snippet blocking_observable.cpp blocking min empty sample
    \snippet output.txt blocking min empty sample

    When the source observable calls on_error:
    \snippet blocking_observable.cpp blocking min error sample
    \snippet output.txt blocking min error sample
*/
    T min() const {
        return source.min().as_blocking().last();
    }
};

namespace detail {
    
template<class SourceOperator, class Subscriber>
struct safe_subscriber 
{
    safe_subscriber(SourceOperator& so, Subscriber& o) : so(std::addressof(so)), o(std::addressof(o)) {}

    void subscribe() {
        try {
            so->on_subscribe(*o);
        }
        catch(...) {
            if (!o->is_subscribed()) {
                throw;
            }
            o->on_error(std::current_exception());
            o->unsubscribe();
        }
    }

    void operator()(const rxsc::schedulable&) {
        subscribe();
    }

    SourceOperator* so;
    Subscriber* o;
};

}

template<>
class observable<void, void>;

/*!
    \defgroup group-observable Observables

    \brief These are the set of observable classes in rxcpp.

    \class rxcpp::observable

    \ingroup group-observable group-core

    \brief a source of values. subscribe or use one of the operator methods that return a new observable, which uses this observable as a source.

    \par Some code
    This sample will observable::subscribe() to values from a observable<void, void>::range().

    \sample
    \snippet range.cpp range sample
    \snippet output.txt range sample

*/
template<class T, class SourceOperator>
class observable
    : public observable_base<T>
{
    static_assert(std::is_same<T, typename SourceOperator::value_type>::value, "SourceOperator::value_type must be the same as T in observable<T, SourceOperator>");

    typedef observable<T, SourceOperator> this_type;

public:
    typedef rxu::decay_t<SourceOperator> source_operator_type;
    mutable source_operator_type source_operator;

private:

    template<class U, class SO>
    friend class observable;

    template<class U, class SO>
    friend bool operator==(const observable<U, SO>&, const observable<U, SO>&);

    template<class Subscriber>
    auto detail_subscribe(Subscriber o) const
        -> composite_subscription {

        typedef rxu::decay_t<Subscriber> subscriber_type;

        static_assert(is_subscriber<subscriber_type>::value, "subscribe must be passed a subscriber");
        static_assert(std::is_same<typename source_operator_type::value_type, T>::value && std::is_convertible<T*, typename subscriber_type::value_type*>::value, "the value types in the sequence must match or be convertible");
        static_assert(detail::has_on_subscribe_for<subscriber_type, source_operator_type>::value, "inner must have on_subscribe method that accepts this subscriber ");

        trace_activity().subscribe_enter(*this, o);

        if (!o.is_subscribed()) {
            trace_activity().subscribe_return(*this);
            return o.get_subscription();
        }

        detail::safe_subscriber<source_operator_type, subscriber_type> subscriber(source_operator, o);

        // make sure to let current_thread take ownership of the thread as early as possible.
        if (rxsc::current_thread::is_schedule_required()) {
            const auto& sc = rxsc::make_current_thread();
            sc.create_worker(o.get_subscription()).schedule(subscriber);
        } else {
            // current_thread already owns this thread.
            subscriber.subscribe();
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

    /*! Return a new observable that performs type-forgetting conversion of this observable.

        \return  The source observable converted to observable<T>.

        \note This operator could be useful to workaround lambda deduction bug on msvc 2013.

        \sample
        \snippet as_dynamic.cpp as_dynamic sample
        \snippet output.txt as_dynamic sample
    */
    template<class... AN>
    observable<T> as_dynamic(AN**...) const {
        return *this;
        static_assert(sizeof...(AN) == 0, "as_dynamic() was passed too many arguments.");
    }

    /*! Return a new observable that contains the blocking methods for this observable.

        \return  An observable that contains the blocking methods for this observable.

        \sample
        \snippet from.cpp threaded from sample
        \snippet output.txt threaded from sample
    */
    template<class... AN>
    blocking_observable<T, this_type> as_blocking(AN**...) const {
        return blocking_observable<T, this_type>(*this);
        static_assert(sizeof...(AN) == 0, "as_blocking() was passed too many arguments.");
    }

    /// \cond SHOW_SERVICE_MEMBERS

    ///
    /// takes any function that will take this observable and produce a result value.
    /// this is intended to allow externally defined operators, that use subscribe,
    /// to be connected into the expression.
    ///
    template<class OperatorFactory>
    auto op(OperatorFactory&& of) const
        -> decltype(of(*(const this_type*)nullptr)) {
        return      of(*this);
        static_assert(is_operator_factory_for<this_type, OperatorFactory>::value, "Function passed for op() must have the signature Result(SourceObservable)");
    }

    ///
    /// takes any function that will take a subscriber for this observable and produce a subscriber.
    /// this is intended to allow externally defined operators, that use make_subscriber, to be connected
    /// into the expression.
    ///
    template<class ResultType, class Operator>
    auto lift(Operator&& op) const
        ->      observable<rxu::value_type_t<rxo::detail::lift_operator<ResultType, source_operator_type, Operator>>, rxo::detail::lift_operator<ResultType, source_operator_type, Operator>> {
        return  observable<rxu::value_type_t<rxo::detail::lift_operator<ResultType, source_operator_type, Operator>>, rxo::detail::lift_operator<ResultType, source_operator_type, Operator>>(
                                                                                                                      rxo::detail::lift_operator<ResultType, source_operator_type, Operator>(source_operator, std::forward<Operator>(op)));
        static_assert(detail::is_lift_function_for<T, subscriber<ResultType>, Operator>::value, "Function passed for lift() must have the signature subscriber<...>(subscriber<T, ...>)");
    }

    ///
    /// takes any function that will take a subscriber for this observable and produce a subscriber.
    /// this is intended to allow externally defined operators, that use make_subscriber, to be connected
    /// into the expression.
    ///
    template<class ResultType, class Operator>
    auto lift_if(Operator&& op) const
        -> typename std::enable_if<detail::is_lift_function_for<T, subscriber<ResultType>, Operator>::value,
            observable<rxu::value_type_t<rxo::detail::lift_operator<ResultType, source_operator_type, Operator>>, rxo::detail::lift_operator<ResultType, source_operator_type, Operator>>>::type {
        return  observable<rxu::value_type_t<rxo::detail::lift_operator<ResultType, source_operator_type, Operator>>, rxo::detail::lift_operator<ResultType, source_operator_type, Operator>>(
                                                                                                                      rxo::detail::lift_operator<ResultType, source_operator_type, Operator>(source_operator, std::forward<Operator>(op)));
    }
    ///
    /// takes any function that will take a subscriber for this observable and produce a subscriber.
    /// this is intended to allow externally defined operators, that use make_subscriber, to be connected
    /// into the expression.
    ///
    template<class ResultType, class Operator>
    auto lift_if(Operator&&) const
        -> typename std::enable_if<!detail::is_lift_function_for<T, subscriber<ResultType>, Operator>::value,
            decltype(rxs::from<ResultType>())>::type {
        return       rxs::from<ResultType>();
    }
    /// \endcond

    /*! Subscribe will cause this observable to emit values to the provided subscriber.

        \tparam ArgN  types of the subscriber parameters

        \param an  the parameters for making a subscriber

        \return  A subscription with which the observer can stop receiving items before the observable has finished sending them.

        The arguments of subscribe are forwarded to rxcpp::make_subscriber function. Some possible alternatives are:

        - Pass an already composed rxcpp::subscriber:
        \snippet subscribe.cpp subscribe by subscriber
        \snippet output.txt subscribe by subscriber

        - Pass an rxcpp::observer. This allows subscribing the same subscriber to several observables:
        \snippet subscribe.cpp subscribe by observer
        \snippet output.txt subscribe by observer

        - Pass an `on_next` handler:
        \snippet subscribe.cpp subscribe by on_next
        \snippet output.txt subscribe by on_next

        - Pass `on_next` and `on_error` handlers:
        \snippet subscribe.cpp subscribe by on_next and on_error
        \snippet output.txt subscribe by on_next and on_error

        - Pass `on_next` and `on_completed` handlers:
        \snippet subscribe.cpp subscribe by on_next and on_completed
        \snippet output.txt subscribe by on_next and on_completed

        - Pass `on_next`, `on_error`, and `on_completed` handlers:
        \snippet subscribe.cpp subscribe by on_next, on_error, and on_completed
        \snippet output.txt subscribe by on_next, on_error, and on_completed
        .

        All the alternatives above also support passing rxcpp::composite_subscription instance. For example:
        \snippet subscribe.cpp subscribe by subscription, on_next, and on_completed
        \snippet output.txt subscribe by subscription, on_next, and on_completed

        If neither subscription nor subscriber are provided, then a new subscription is created and returned as a result:
        \snippet subscribe.cpp subscribe unsubscribe
        \snippet output.txt subscribe unsubscribe

        For more details, see rxcpp::make_subscriber function description.
    */
    template<class... ArgN>
    auto subscribe(ArgN&&... an) const
        -> composite_subscription {
        return detail_subscribe(make_subscriber<T>(std::forward<ArgN>(an)...));
    }

    /*! @copydoc rx-all.hpp
     */
    template<class... AN>
    auto all(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(all_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(all_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rxcpp::operators::exists
     */
    template<class... AN>
    auto exists(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(exists_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(exists_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rxcpp::operators::contains
     */
    template<class... AN>
    auto contains(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(contains_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(contains_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-filter.hpp
     */
    template<class... AN>
    auto filter(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(filter_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(filter_tag{}, *this,                std::forward<AN>(an)...);
    }

    /*! If the source Observable terminates without emitting any items, emits items from a backup Observable.

        \tparam BackupSource  the type of the backup observable.

        \param t  a backup observable that is used if the source observable is empty.

        \return  Observable that emits items from a backup observable if the source observable is empty.

        \sample
        \snippet switch_if_empty.cpp switch_if_empty sample
        \snippet output.txt switch_if_empty sample
    */
    template<class BackupSource>
    auto switch_if_empty(BackupSource t) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> typename std::enable_if<is_observable<BackupSource>::value,
            decltype(EXPLICIT_THIS lift<T>(rxo::detail::switch_if_empty<T, BackupSource>(std::move(t))))>::type
    /// \endcond
    {
        return                    lift<T>(rxo::detail::switch_if_empty<T, BackupSource>(std::move(t)));
    }

    /*! If the source Observable terminates without emitting any items, emits a default item and completes.

        \tparam V  the type of the value to emit.

        \param v  the default value to emit

        \return  Observable that emits the specified default item if the source observable is empty.

        \sample
        \snippet default_if_empty.cpp default_if_empty sample
        \snippet output.txt default_if_empty sample
    */
    template <typename V>
    auto default_if_empty(V v) const
      -> decltype(EXPLICIT_THIS switch_if_empty(rxs::from(std::move(v))))
    {
        return                  switch_if_empty(rxs::from(std::move(v)));
    }

    /*! Determine whether two Observables emit the same sequence of items.

        \tparam OtherSource      the type of the other observable.
        \tparam BinaryPredicate  the type of the value comparing function. The signature should be equivalent to the following: bool pred(const T1& a, const T2& b);
        \tparam Coordination  the type of the scheduler.

        \param t     the other Observable that emits items to compare.
        \param pred  the function that implements comparison of two values.
        \param cn    the scheduler.

        \return  Observable that emits true only if both sequences terminate normally after emitting the same sequence of items in the same order; otherwise it will emit false.

        \sample
        \snippet sequence_equal.cpp sequence_equal sample
        \snippet output.txt sequence_equal sample
    */
    template<class OtherSource, class BinaryPredicate, class Coordination>
    auto sequence_equal(OtherSource&& t, BinaryPredicate&& pred, Coordination&& cn) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> typename std::enable_if<is_observable<OtherSource>::value,
                observable<bool, rxo::detail::sequence_equal<T, this_type, OtherSource, BinaryPredicate, Coordination>>>::type
    /// \endcond
    {
        return  observable<bool, rxo::detail::sequence_equal<T, this_type, OtherSource, BinaryPredicate, Coordination>>(
                rxo::detail::sequence_equal<T, this_type, OtherSource, BinaryPredicate, Coordination>(*this, std::forward<OtherSource>(t), std::forward<BinaryPredicate>(pred), std::forward<Coordination>(cn)));
    }


    /*! Determine whether two Observables emit the same sequence of items.

        \tparam OtherSource      the type of the other observable.
        \tparam BinaryPredicate  the type of the value comparing function. The signature should be equivalent to the following: bool pred(const T1& a, const T2& b);

        \param t     the other Observable that emits items to compare.
        \param pred  the function that implements comparison of two values.

        \return  Observable that emits true only if both sequences terminate normally after emitting the same sequence of items in the same order; otherwise it will emit false.

        \sample
        \snippet sequence_equal.cpp sequence_equal sample
        \snippet output.txt sequence_equal sample
    */
    template<class OtherSource, class BinaryPredicate>
    auto sequence_equal(OtherSource&& t, BinaryPredicate&& pred) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> typename std::enable_if<is_observable<OtherSource>::value && !is_coordination<BinaryPredicate>::value,
                observable<bool, rxo::detail::sequence_equal<T, this_type, OtherSource, BinaryPredicate, identity_one_worker>>>::type
    /// \endcond
    {
        return  observable<bool, rxo::detail::sequence_equal<T, this_type, OtherSource, BinaryPredicate, identity_one_worker>>(
                rxo::detail::sequence_equal<T, this_type, OtherSource, BinaryPredicate, identity_one_worker>(*this, std::forward<OtherSource>(t), std::forward<BinaryPredicate>(pred), identity_one_worker(rxsc::make_current_thread())));
    }

    /*! Determine whether two Observables emit the same sequence of items.

        \tparam OtherSource   the type of the other observable.
        \tparam Coordination  the type of the scheduler.

        \param t  the other Observable that emits items to compare.
        \param cn the scheduler.

        \return  Observable that emits true only if both sequences terminate normally after emitting the same sequence of items in the same order; otherwise it will emit false.

        \sample
        \snippet sequence_equal.cpp sequence_equal sample
        \snippet output.txt sequence_equal sample
    */
    template<class OtherSource, class Coordination>
    auto sequence_equal(OtherSource&& t, Coordination&& cn) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> typename std::enable_if<is_observable<OtherSource>::value && is_coordination<Coordination>::value,
                observable<bool, rxo::detail::sequence_equal<T, this_type, OtherSource, rxu::equal_to<>, Coordination>>>::type
    /// \endcond
    {
        return  observable<bool, rxo::detail::sequence_equal<T, this_type, OtherSource, rxu::equal_to<>, Coordination>>(
                rxo::detail::sequence_equal<T, this_type, OtherSource, rxu::equal_to<>, Coordination>(*this, std::forward<OtherSource>(t), rxu::equal_to<>(), std::forward<Coordination>(cn)));
    }

    /*! Determine whether two Observables emit the same sequence of items.

        \tparam OtherSource  the type of the other observable.

        \param t  the other Observable that emits items to compare.

        \return  Observable that emits true only if both sequences terminate normally after emitting the same sequence of items in the same order; otherwise it will emit false.

        \sample
        \snippet sequence_equal.cpp sequence_equal sample
        \snippet output.txt sequence_equal sample
    */
    template<class OtherSource>
    auto sequence_equal(OtherSource&& t) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> typename std::enable_if<is_observable<OtherSource>::value,
                observable<bool, rxo::detail::sequence_equal<T, this_type, OtherSource, rxu::equal_to<>, identity_one_worker>>>::type
    /// \endcond
    {
        return  observable<bool, rxo::detail::sequence_equal<T, this_type, OtherSource, rxu::equal_to<>, identity_one_worker>>(
                rxo::detail::sequence_equal<T, this_type, OtherSource, rxu::equal_to<>, identity_one_worker>(*this, std::forward<OtherSource>(t), rxu::equal_to<>(), identity_one_worker(rxsc::make_current_thread())));
    }

    /*! inspect calls to on_next, on_error and on_completed.

        \tparam MakeObserverArgN...  these args are passed to make_observer

        \param an  these args are passed to make_observer.

        \return  Observable that emits the same items as the source observable to both the subscriber and the observer.

        \note If an on_error method is not supplied the observer will ignore errors rather than call std::terminate()

        \sample
        \snippet tap.cpp tap sample
        \snippet output.txt tap sample

        If the source observable generates an error, the observer passed to tap is called:
        \snippet tap.cpp error tap sample
        \snippet output.txt error tap sample
    */
    template<class... MakeObserverArgN>
    auto tap(MakeObserverArgN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::tap<T, std::tuple<MakeObserverArgN...>>(std::make_tuple(std::forward<MakeObserverArgN>(an)...))))
        /// \endcond
    {
        return                    lift<T>(rxo::detail::tap<T, std::tuple<MakeObserverArgN...>>(std::make_tuple(std::forward<MakeObserverArgN>(an)...)));
    }

    /*! Returns an observable that emits indications of the amount of time lapsed between consecutive emissions of the source observable.
        The first emission from this new Observable indicates the amount of time lapsed between the time when the observer subscribed to the Observable and the time when the source Observable emitted its first item.

        \tparam Coordination  the type of the scheduler

        \param coordination  the scheduler for itme intervals

        \return  Observable that emits a time_duration to indicate the amount of time lapsed between pairs of emissions.

        \sample
        \snippet time_interval.cpp time_interval sample
        \snippet output.txt time_interval sample
    */
    template<class Coordination>
    auto time_interval(Coordination coordination) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(EXPLICIT_THIS lift<rxsc::scheduler::clock_type::time_point::duration>(rxo::detail::time_interval<T, Coordination>{coordination}))
    /// \endcond
    {
        return                lift<rxsc::scheduler::clock_type::time_point::duration>(rxo::detail::time_interval<T, Coordination>{coordination});
    }

    /*! Returns an observable that emits indications of the amount of time lapsed between consecutive emissions of the source observable.
        The first emission from this new Observable indicates the amount of time lapsed between the time when the observer subscribed to the Observable and the time when the source Observable emitted its first item.

        \return  Observable that emits a time_duration to indicate the amount of time lapsed between pairs of emissions.

        \sample
        \snippet time_interval.cpp time_interval sample
        \snippet output.txt time_interval sample
    */
    template<class... AN>
    auto time_interval(AN**...) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(EXPLICIT_THIS lift<rxsc::scheduler::clock_type::time_point::duration>(rxo::detail::time_interval<T, identity_one_worker>{identity_current_thread()}))
    /// \endcond
    {
        return                lift<rxsc::scheduler::clock_type::time_point::duration>(rxo::detail::time_interval<T, identity_one_worker>{identity_current_thread()});
        static_assert(sizeof...(AN) == 0, "time_interval() was passed too many arguments.");
    }

    /*! Return an observable that terminates with timeout_error if a particular timespan has passed without emitting another item from the source observable.

        \tparam Duration      the type of time interval
        \tparam Coordination  the type of the scheduler

        \param period        the period of time wait for another item from the source observable.
        \param coordination  the scheduler to manage timeout for each event

        \return  Observable that terminates with an error if a particular timespan has passed without emitting another item from the source observable.

        \sample
        \snippet timeout.cpp timeout sample
        \snippet output.txt timeout sample
    */
    template<class Duration, class Coordination>
    auto timeout(Duration period, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::timeout<T, Duration, Coordination>(period, coordination)))
        /// \endcond
    {
        return                    lift<T>(rxo::detail::timeout<T, Duration, Coordination>(period, coordination));
    }

    /*! Return an observable that terminates with timeout_error if a particular timespan has passed without emitting another item from the source observable.

        \tparam Duration      the type of time interval

        \param period        the period of time wait for another item from the source observable.

        \return  Observable that terminates with an error if a particular timespan has passed without emitting another item from the source observable.

        \sample
        \snippet timeout.cpp timeout sample
        \snippet output.txt timeout sample
    */
    template<class Duration>
    auto timeout(Duration period) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::timeout<T, Duration, identity_one_worker>(period, identity_current_thread())))
        /// \endcond
    {
        return                    lift<T>(rxo::detail::timeout<T, Duration, identity_one_worker>(period, identity_current_thread()));
    }

    /*! Returns an observable that attaches a timestamp to each item emitted by the source observable indicating when it was emitted.

        \tparam Coordination  the type of the scheduler

        \param coordination  the scheduler to manage timeout for each event

        \return  Observable that emits a pair: { item emitted by the source observable, time_point representing the current value of the clock }.

        \sample
        \snippet timestamp.cpp timestamp sample
        \snippet output.txt timestamp sample
    */
    template<class Coordination>
    auto timestamp(Coordination coordination) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(EXPLICIT_THIS lift<std::pair<T, rxsc::scheduler::clock_type::time_point>>(rxo::detail::timestamp<T, Coordination>{coordination}))
    /// \endcond
    {
        return                lift<std::pair<T, rxsc::scheduler::clock_type::time_point>>(rxo::detail::timestamp<T, Coordination>{coordination});
    }

    /*! Returns an observable that attaches a timestamp to each item emitted by the source observable indicating when it was emitted.

        \tparam ClockType    the type of the clock to return a time_point.

        \return  Observable that emits a pair: { item emitted by the source observable, time_point representing the current value of the clock }.

        \sample
        \snippet timestamp.cpp timestamp sample
        \snippet output.txt timestamp sample
    */
    template<class... AN>
    auto timestamp(AN**...) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(EXPLICIT_THIS lift<std::pair<T, rxsc::scheduler::clock_type::time_point>>(rxo::detail::timestamp<T, identity_one_worker>{identity_current_thread()}))
    /// \endcond
    {
        return                lift<std::pair<T, rxsc::scheduler::clock_type::time_point>>(rxo::detail::timestamp<T, identity_one_worker>{identity_current_thread()});
        static_assert(sizeof...(AN) == 0, "timestamp() was passed too many arguments.");
    }

    /*! Add a new action at the end of the new observable that is returned.

        \tparam LastCall  the type of the action function

        \param lc  the action function

        \return  Observable that emits the same items as the source observable, then invokes the given action.

        \sample
        \snippet finally.cpp finally sample
        \snippet output.txt finally sample

        If the source observable generates an error, the final action is still being called:
        \snippet finally.cpp error finally sample
        \snippet output.txt error finally sample
    */
    template<class LastCall>
    auto finally(LastCall lc) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::finally<T, LastCall>(std::move(lc))))
        /// \endcond
    {
        return                    lift<T>(rxo::detail::finally<T, LastCall>(std::move(lc)));
    }

    /*! If an error occurs, take the result from the Selector and subscribe to that instead.

        \tparam Selector  the actual type of a function of the form `observable<T>(std::exception_ptr)`

        \param s  the function of the form `observable<T>(std::exception_ptr)`

        \return  Observable that emits the items from the source observable and switches to a new observable on error.

        \sample
        \snippet on_error_resume_next.cpp on_error_resume_next sample
        \snippet output.txt on_error_resume_next sample
    */
    template<class Selector>
    auto on_error_resume_next(Selector s) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<rxu::value_type_t<rxo::detail::on_error_resume_next<T, Selector>>>(rxo::detail::on_error_resume_next<T, Selector>(std::move(s))))
        /// \endcond
    {
        return                    lift<rxu::value_type_t<rxo::detail::on_error_resume_next<T, Selector>>>(rxo::detail::on_error_resume_next<T, Selector>(std::move(s)));
    }

    /*! For each item from this observable use Selector to produce an item to emit from the new observable that is returned.

        \tparam Selector  the type of the transforming function

        \param s  the selector function

        \return  Observable that emits the items from the source observable, transformed by the specified function.

        \sample
        \snippet map.cpp map sample
        \snippet output.txt map sample
    */
    template<class Selector>
    auto map(Selector s) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<rxu::value_type_t<rxo::detail::map<T, Selector>>>(rxo::detail::map<T, Selector>(std::move(s))))
        /// \endcond
    {
        return                    lift<rxu::value_type_t<rxo::detail::map<T, Selector>>>(rxo::detail::map<T, Selector>(std::move(s)));
    }

    /*! @copydoc rx-debounce.hpp
     */
    template<class... AN>
    auto debounce(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(debounce_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(debounce_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-delay.hpp
     */
    template<class... AN>
    auto delay(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(delay_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(delay_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-distinct.hpp
     */
    template<class... AN>
    auto distinct(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(distinct_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(distinct_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-distinct_until_changed.hpp
     */
    template<class... AN>
    auto distinct_until_changed(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(distinct_until_changed_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(distinct_until_changed_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-element_at.hpp
     */
    template<class... AN>
    auto element_at(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(element_at_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(element_at_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! Return an observable that emits connected, non-overlapping windows, each containing at most count items from the source observable.

        \param count  the maximum size of each window before it should be completed

        \return  Observable that emits connected, non-overlapping windows, each containing at most count items from the source observable.

        \sample
        \snippet window.cpp window count sample
        \snippet output.txt window count sample
    */
    template<class... AN>
    auto window(int count, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window<T>(count, count)))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window<T>(count, count));
        static_assert(sizeof...(AN) == 0, "window(count) was passed too many arguments.");
    }

    /*! Return an observable that emits windows every skip items containing at most count items from the source observable.

        \param count  the maximum size of each window before it should be completed
        \param skip   how many items need to be skipped before starting a new window

        \return  Observable that emits windows every skip items containing at most count items from the source observable.

        \sample
        \snippet window.cpp window count+skip sample
        \snippet output.txt window count+skip sample
    */
    template<class... AN>
    auto window(int count, int skip, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window<T>(count, skip)))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window<T>(count, skip));
        static_assert(sizeof...(AN) == 0, "window(count, skip) was passed too many arguments.");
    }

    /*! Return an observable that emits observables every skip time interval and collects items from this observable for period of time into each produced observable, on the specified scheduler.

        \tparam Duration      the type of time intervals
        \tparam Coordination  the type of the scheduler

        \param period        the period of time each window collects items before it is completed
        \param skip          the period of time after which a new window will be created
        \param coordination  the scheduler for the windows

        \return  Observable that emits observables every skip time interval and collect items from this observable for period of time into each produced observable.

        \sample
        \snippet window.cpp window period+skip+coordination sample
        \snippet output.txt window period+skip+coordination sample
    */
    template<class Duration, class Coordination>
    auto window_with_time(Duration period, Duration skip, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window_with_time<T, Duration, Coordination>(period, skip, coordination)))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window_with_time<T, Duration, Coordination>(period, skip, coordination));
    }

    /*! Return an observable that emits observables every skip time interval and collects items from this observable for period of time into each produced observable.

        \tparam Duration  the type of time intervals

        \param period  the period of time each window collects items before it is completed
        \param skip    the period of time after which a new window will be created

        \return  Observable that emits observables every skip time interval and collect items from this observable for period of time into each produced observable.

        \sample
        \snippet window.cpp window period+skip sample
        \snippet output.txt window period+skip sample
    */
    template<class Duration>
    auto window_with_time(Duration period, Duration skip) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window_with_time<T, Duration, identity_one_worker>(period, skip, identity_current_thread())))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window_with_time<T, Duration, identity_one_worker>(period, skip, identity_current_thread()));
    }

    /*! Return an observable that emits observables every period time interval and collects items from this observable for period of time into each produced observable, on the specified scheduler.

        \tparam Duration      the type of time intervals
        \tparam Coordination  the type of the scheduler

        \param period        the period of time each window collects items before it is completed and replaced with a new window
        \param coordination  the scheduler for the windows

        \return  Observable that emits observables every period time interval and collect items from this observable for period of time into each produced observable.

        \sample
        \snippet window.cpp window period+coordination sample
        \snippet output.txt window period+coordination sample
    */
    template<class Duration, class Coordination, class Reqiures = typename rxu::types_checked_from<typename Coordination::coordination_tag>::type>
    auto window_with_time(Duration period, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window_with_time<T, Duration, Coordination>(period, period, coordination)))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window_with_time<T, Duration, Coordination>(period, period, coordination));
    }

    /*! Return an observable that emits connected, non-overlapping windows represending items emitted by the source observable during fixed, consecutive durations.

        \tparam Duration  the type of time intervals

        \param period  the period of time each window collects items before it is completed and replaced with a new window

        \return  Observable that emits connected, non-overlapping windows represending items emitted by the source observable during fixed, consecutive durations.

        \sample
        \snippet window.cpp window period sample
        \snippet output.txt window period sample
    */
    template<class Duration>
    auto window_with_time(Duration period) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window_with_time<T, Duration, identity_one_worker>(period, period, identity_current_thread())))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window_with_time<T, Duration, identity_one_worker>(period, period, identity_current_thread()));
    }

    /*! Return an observable that emits connected, non-overlapping windows of items from the source observable that were emitted during a fixed duration of time or when the window has reached maximum capacity (whichever occurs first), on the specified scheduler.

        \tparam Duration      the type of time intervals
        \tparam Coordination  the type of the scheduler

        \param period        the period of time each window collects items before it is completed and replaced with a new window
        \param count         the maximum size of each window before it is completed and new window is created
        \param coordination  the scheduler for the windows

        \return  Observable that emits connected, non-overlapping windows of items from the source observable that were emitted during a fixed duration of time or when the window has reached maximum capacity (whichever occurs first).

        \sample
        \snippet window.cpp window period+count+coordination sample
        \snippet output.txt window period+count+coordination sample
    */
    template<class Duration, class Coordination>
    auto window_with_time_or_count(Duration period, int count, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window_with_time_or_count<T, Duration, Coordination>(period, count, coordination)))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window_with_time_or_count<T, Duration, Coordination>(period, count, coordination));
    }

    /*! Return an observable that emits connected, non-overlapping windows of items from the source observable that were emitted during a fixed duration of time or when the window has reached maximum capacity (whichever occurs first).

        \tparam Duration  the type of time intervals

        \param period  the period of time each window collects items before it is completed and replaced with a new window
        \param count   the maximum size of each window before it is completed and new window is created

        \return  Observable that emits connected, non-overlapping windows of items from the source observable that were emitted during a fixed duration of time or when the window has reached maximum capacity (whichever occurs first).

        \sample
        \snippet window.cpp window period+count sample
        \snippet output.txt window period+count sample
    */
    template<class Duration>
    auto window_with_time_or_count(Duration period, int count) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window_with_time_or_count<T, Duration, identity_one_worker>(period, count, identity_current_thread())))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window_with_time_or_count<T, Duration, identity_one_worker>(period, count, identity_current_thread()));
    }

    /*! Return an observable that emits observables every period time interval and collects items from this observable for period of time into each produced observable, on the specified scheduler.

        \tparam Openings        observable<OT>
        \tparam ClosingSelector a function of type observable<CT>(OT)
        \tparam Coordination    the type of the scheduler

        \param opens         each value from this observable opens a new window.
        \param closes        this function is called for each opened window and returns an observable. the first value from the returned observable will close the window
        \param coordination  the scheduler for the windows

        \return  Observable that emits an observable for each opened window.

        \sample
        \snippet window.cpp window toggle+coordination sample
        \snippet output.txt window toggle+coordination sample
    */
    template<class Openings, class ClosingSelector, class Coordination, class Reqiures = typename rxu::types_checked_from<typename Coordination::coordination_tag>::type>
    auto window_toggle(Openings opens, ClosingSelector closes, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window_toggle<T, Openings, ClosingSelector, Coordination>(opens, closes, coordination)))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window_toggle<T, Openings, ClosingSelector, Coordination>(opens, closes, coordination));
    }

    /*! Return an observable that emits connected, non-overlapping windows represending items emitted by the source observable during fixed, consecutive durations.

        \tparam Openings        observable<OT>
        \tparam ClosingSelector a function of type observable<CT>(OT)

        \param opens         each value from this observable opens a new window.
        \param closes        this function is called for each opened window and returns an observable. the first value from the returned observable will close the window

        \return  Observable that emits an observable for each opened window.

        \sample
        \snippet window.cpp window toggle sample
        \snippet output.txt window toggle sample
    */
    template<class Openings, class ClosingSelector>
    auto window_toggle(Openings opens, ClosingSelector closes) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<observable<T>>(rxo::detail::window_toggle<T, Openings, ClosingSelector, identity_one_worker>(opens, closes, identity_current_thread())))
        /// \endcond
    {
        return                    lift<observable<T>>(rxo::detail::window_toggle<T, Openings, ClosingSelector, identity_one_worker>(opens, closes, identity_current_thread()));
    }

    /*! Return an observable that emits connected, non-overlapping buffer, each containing at most count items from the source observable.

        \param count  the maximum size of each buffer before it should be emitted

        \return  Observable that emits connected, non-overlapping buffers, each containing at most count items from the source observable.

        \sample
        \snippet buffer.cpp buffer count sample
        \snippet output.txt buffer count sample
    */
    template<class... AN>
    auto buffer(int count, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift_if<std::vector<T>>(rxo::detail::buffer_count<T>(count, count)))
        /// \endcond
    {
        return                    lift_if<std::vector<T>>(rxo::detail::buffer_count<T>(count, count));
        static_assert(sizeof...(AN) == 0, "buffer(count) was passed too many arguments.");
    }

    /*! Return an observable that emits buffers every skip items containing at most count items from the source observable.

        \param count  the maximum size of each buffers before it should be emitted
        \param skip   how many items need to be skipped before starting a new buffers

        \return  Observable that emits buffers every skip items containing at most count items from the source observable.

        \sample
        \snippet buffer.cpp buffer count+skip sample
        \snippet output.txt buffer count+skip sample
    */
    template<class... AN>
    auto buffer(int count, int skip, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift_if<std::vector<T>>(rxo::detail::buffer_count<T>(count, skip)))
        /// \endcond
    {
        return                    lift_if<std::vector<T>>(rxo::detail::buffer_count<T>(count, skip));
        static_assert(sizeof...(AN) == 0, "buffer(count, skip) was passed too many arguments.");
    }

    /*! Return an observable that emits buffers every skip time interval and collects items from this observable for period of time into each produced buffer, on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param period        the period of time each buffer collects items before it is emitted
        \param skip          the period of time after which a new buffer will be created
        \param coordination  the scheduler for the buffers

        \return  Observable that emits buffers every skip time interval and collect items from this observable for period of time into each produced buffer.

        \sample
        \snippet buffer.cpp buffer period+skip+coordination sample
        \snippet output.txt buffer period+skip+coordination sample
    */
    template<class Coordination>
    auto buffer_with_time(rxsc::scheduler::clock_type::duration period, rxsc::scheduler::clock_type::duration skip, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift_if<std::vector<T>>(rxo::detail::buffer_with_time<T, rxsc::scheduler::clock_type::duration, Coordination>(period, skip, coordination)))
        /// \endcond
    {
        return                    lift_if<std::vector<T>>(rxo::detail::buffer_with_time<T, rxsc::scheduler::clock_type::duration, Coordination>(period, skip, coordination));
    }

    /*! Return an observable that emits buffers every skip time interval and collects items from this observable for period of time into each produced buffer.

        \param period        the period of time each buffer collects items before it is emitted
        \param skip          the period of time after which a new buffer will be created

        \return  Observable that emits buffers every skip time interval and collect items from this observable for period of time into each produced buffer.

        \sample
        \snippet buffer.cpp buffer period+skip sample
        \snippet output.txt buffer period+skip sample

        Overlapping buffers are allowed:
        \snippet buffer.cpp buffer period+skip overlapping sample
        \snippet output.txt buffer period+skip overlapping sample

        If no items are emitted, an empty buffer is returned:
        \snippet buffer.cpp buffer period+skip empty sample
        \snippet output.txt buffer period+skip empty sample
    */
    template<class Duration>
    auto buffer_with_time(Duration period, Duration skip) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift_if<std::vector<T>>(rxo::detail::buffer_with_time<T, Duration, identity_one_worker>(period, skip, identity_current_thread())))
        /// \endcond
    {
        return                    lift_if<std::vector<T>>(rxo::detail::buffer_with_time<T, Duration, identity_one_worker>(period, skip, identity_current_thread()));
    }

    /*! Return an observable that emits buffers every period time interval and collects items from this observable for period of time into each produced buffer, on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param period        the period of time each buffer collects items before it is emitted and replaced with a new buffer
        \param coordination  the scheduler for the buffers

        \return  Observable that emits buffers every period time interval and collect items from this observable for period of time into each produced buffer.

        \sample
        \snippet buffer.cpp buffer period+coordination sample
        \snippet output.txt buffer period+coordination sample
    */
    template<class Coordination,
        class Requires = typename std::enable_if<is_coordination<Coordination>::value, rxu::types_checked>::type>
    auto buffer_with_time(rxsc::scheduler::clock_type::duration period, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift_if<std::vector<T>>(rxo::detail::buffer_with_time<T, rxsc::scheduler::clock_type::duration, Coordination>(period, period, coordination)))
        /// \endcond
    {
        return                    lift_if<std::vector<T>>(rxo::detail::buffer_with_time<T, rxsc::scheduler::clock_type::duration, Coordination>(period, period, coordination));
    }

    /*! Return an observable that emits buffers every period time interval and collects items from this observable for period of time into each produced buffer.

        \param period  the period of time each buffer collects items before it is emitted and replaced with a new buffer

        \return  Observable that emits buffers every period time interval and collect items from this observable for period of time into each produced buffer.

        \sample
        \snippet buffer.cpp buffer period sample
        \snippet output.txt buffer period sample
    */
    template<class Duration>
    auto buffer_with_time(Duration period) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift_if<std::vector<T>>(rxo::detail::buffer_with_time<T, Duration, identity_one_worker>(period, period, identity_current_thread())))
        /// \endcond
    {
        return                    lift_if<std::vector<T>>(rxo::detail::buffer_with_time<T, Duration, identity_one_worker>(period, period, identity_current_thread()));
    }

    /*! Return an observable that emits connected, non-overlapping buffers of items from the source observable that were emitted during a fixed duration of time or when the buffer has reached maximum capacity (whichever occurs first), on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param period        the period of time each buffer collects items before it is emitted and replaced with a new buffer
        \param count         the maximum size of each buffer before it is emitted and new buffer is created
        \param coordination  the scheduler for the buffers

        \return  Observable that emits connected, non-overlapping buffers of items from the source observable that were emitted during a fixed duration of time or when the buffer has reached maximum capacity (whichever occurs first).

        \sample
        \snippet buffer.cpp buffer period+count+coordination sample
        \snippet output.txt buffer period+count+coordination sample
    */
    template<class Coordination>
    auto buffer_with_time_or_count(rxsc::scheduler::clock_type::duration period, int count, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift_if<std::vector<T>>(rxo::detail::buffer_with_time_or_count<T, rxsc::scheduler::clock_type::duration, Coordination>(period, count, coordination)))
        /// \endcond
    {
        return                    lift_if<std::vector<T>>(rxo::detail::buffer_with_time_or_count<T, rxsc::scheduler::clock_type::duration, Coordination>(period, count, coordination));
    }

    /*! Return an observable that emits connected, non-overlapping buffers of items from the source observable that were emitted during a fixed duration of time or when the buffer has reached maximum capacity (whichever occurs first).

        \param period        the period of time each buffer collects items before it is emitted and replaced with a new buffer
        \param count         the maximum size of each buffer before it is emitted and new buffer is created

        \return  Observable that emits connected, non-overlapping buffers of items from the source observable that were emitted during a fixed duration of time or when the buffer has reached maximum capacity (whichever occurs first).

        \sample
        \snippet buffer.cpp buffer period+count sample
        \snippet output.txt buffer period+count sample
    */
    template<class Duration>
    auto buffer_with_time_or_count(Duration period, int count) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift_if<std::vector<T>>(rxo::detail::buffer_with_time_or_count<T, Duration, identity_one_worker>(period, count, identity_current_thread())))
        /// \endcond
    {
        return                    lift_if<std::vector<T>>(rxo::detail::buffer_with_time_or_count<T, Duration, identity_one_worker>(period, count, identity_current_thread()));
    }

    /// \cond SHOW_SERVICE_MEMBERS
    template<class Coordination>
    struct defer_switch_on_next : public defer_observable<
        is_observable<value_type>,
        this_type,
        rxo::detail::switch_on_next, value_type, observable<value_type>, Coordination>
    {
    };
    /// \endcond

    /*! Return observable that emits the items emitted by the observable most recently emitted by the source observable.

        \return  Observable that emits the items emitted by the observable most recently emitted by the source observable.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet switch_on_next.cpp switch_on_next sample
        \snippet output.txt switch_on_next sample
    */
    template<class... AN>
    auto switch_on_next(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename defer_switch_on_next<identity_one_worker>::observable_type
        /// \endcond
    {
        return      defer_switch_on_next<identity_one_worker>::make(*this, *this, identity_current_thread());
        static_assert(sizeof...(AN) == 0, "switch_on_next() was passed too many arguments.");
    }

    /*! Return observable that emits the items emitted by the observable most recently emitted by the source observable, on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param cn  the scheduler to synchronize sources from different contexts

        \return  Observable that emits the items emitted by the observable most recently emitted by the source observable.

        \sample
        \snippet switch_on_next.cpp threaded switch_on_next sample
        \snippet output.txt threaded switch_on_next sample
    */
    template<class Coordination>
    auto switch_on_next(Coordination cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_switch_on_next<Coordination>::value,
            typename    defer_switch_on_next<Coordination>::observable_type>::type
        /// \endcond
    {
        return          defer_switch_on_next<Coordination>::make(*this, *this, std::move(cn));
    }

    /// \cond SHOW_SERVICE_MEMBERS
    template<class Coordination>
    struct defer_merge : public defer_observable<
        is_observable<value_type>,
        this_type,
        rxo::detail::merge, value_type, observable<value_type>, Coordination>
    {
    };
    /// \endcond

    /*! For each item from this observable subscribe.
        For each item from all of the nested observables deliver from the new observable that is returned.

        \return  Observable that emits items that are the result of flattening the observables emitted by the source observable.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet merge.cpp implicit merge sample
        \snippet output.txt implicit merge sample
    */
    template<class... AN>
    auto merge(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename defer_merge<identity_one_worker>::observable_type
        /// \endcond
    {
        return      defer_merge<identity_one_worker>::make(*this, *this, identity_current_thread());
        static_assert(sizeof...(AN) == 0, "merge() was passed too many arguments.");
    }

    /*! For each item from this observable subscribe.
        For each item from all of the nested observables deliver from the new observable that is returned.

        \tparam Coordination  the type of the scheduler

        \param  cn  the scheduler to synchronize sources from different contexts.

        \return  Observable that emits items that are the result of flattening the observables emitted by the source observable.

        \sample
        \snippet merge.cpp threaded implicit merge sample
        \snippet output.txt threaded implicit merge sample
    */
    template<class Coordination>
    auto merge(Coordination cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_merge<Coordination>::value,
            typename    defer_merge<Coordination>::observable_type>::type
        /// \endcond
    {
        return          defer_merge<Coordination>::make(*this, *this, std::move(cn));
    }

    /// \cond SHOW_SERVICE_MEMBERS
    template<class Coordination, class Value0>
    struct defer_merge_from : public defer_observable<
        rxu::all_true<
            is_coordination<Coordination>::value,
            is_observable<Value0>::value>,
        this_type,
        rxo::detail::merge, observable<value_type>, observable<observable<value_type>>, Coordination>
    {
    };
    /// \endcond

    /*! For each given observable subscribe.
        For each emitted item deliver from the new observable that is returned.

        \tparam Value0  ...
        \tparam ValueN  types of source observables

        \param  v0  ...
        \param  vn  source observables

        \return  Observable that emits items that are the result of flattening the observables emitted by the source observable.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet merge.cpp merge sample
        \snippet output.txt merge sample
    */
    template<class Value0, class... ValueN>
    auto merge(Value0 v0, ValueN... vn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_merge_from<identity_one_worker, Value0>::value,
            typename    defer_merge_from<identity_one_worker, Value0>::observable_type>::type
        /// \endcond
    {
        return          defer_merge_from<identity_one_worker, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), identity_current_thread());
    }

    /*! For each given observable subscribe.
        For each emitted item deliver from the new observable that is returned.

        \tparam Coordination  the type of the scheduler
        \tparam Value0        ...
        \tparam ValueN        types of source observables

        \param  cn  the scheduler to synchronize sources from different contexts.
        \param  v0  ...
        \param  vn  source observables

        \return  Observable that emits items that are the result of flattening the observables emitted by the source observable.

        \sample
        \snippet merge.cpp threaded merge sample
        \snippet output.txt threaded merge sample
    */
    template<class Coordination, class Value0, class... ValueN>
    auto merge(Coordination cn, Value0 v0, ValueN... vn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_merge_from<Coordination, Value0>::value,
            typename    defer_merge_from<Coordination, Value0>::observable_type>::type
        /// \endcond
    {
        return          defer_merge_from<Coordination, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), std::move(cn));
    }

    /// \cond SHOW_SERVICE_MEMBERS
    template<class Coordination>
    struct defer_amb : public defer_observable<
        is_observable<value_type>,
        this_type,
        rxo::detail::amb, value_type, observable<value_type>, Coordination>
    {
    };
    /// \endcond

    /*! For each item from only the first of the nested observables deliver from the new observable that is returned.

        \return  Observable that emits the same sequence as whichever of the observables emitted from this observable that first emitted an item or sent a termination notification.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet amb.cpp implicit amb sample
        \snippet output.txt implicit amb sample
    */
    template<class... AN>
    auto amb(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename defer_amb<identity_one_worker>::observable_type
        /// \endcond
    {
        return      defer_amb<identity_one_worker>::make(*this, *this, identity_current_thread());
        static_assert(sizeof...(AN) == 0, "amb() was passed too many arguments.");
    }

    /*! For each item from only the first of the nested observables deliver from the new observable that is returned, on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param  cn  the scheduler to synchronize sources from different contexts.

        \return  Observable that emits the same sequence as whichever of the observables emitted from this observable that first emitted an item or sent a termination notification.

        \sample
        \snippet amb.cpp threaded implicit amb sample
        \snippet output.txt threaded implicit amb sample
    */
    template<class Coordination>
    auto amb(Coordination cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_amb<Coordination>::value,
            typename    defer_amb<Coordination>::observable_type>::type
        /// \endcond
    {
        return          defer_amb<Coordination>::make(*this, *this, std::move(cn));
    }

    /// \cond SHOW_SERVICE_MEMBERS
    template<class Coordination, class Value0>
    struct defer_amb_from : public defer_observable<
        rxu::all_true<
            is_coordination<Coordination>::value,
            is_observable<Value0>::value>,
        this_type,
        rxo::detail::amb, observable<value_type>, observable<observable<value_type>>, Coordination>
    {
    };
    /// \endcond

    /*! For each item from only the first of the given observables deliver from the new observable that is returned.

        \tparam Value0      ...
        \tparam ValueN      types of source observables

        \param  v0  ...
        \param  vn  source observables

        \return  Observable that emits the same sequence as whichever of the source observables first emitted an item or sent a termination notification.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet amb.cpp amb sample
        \snippet output.txt amb sample
    */
    template<class Value0, class... ValueN>
    auto amb(Value0 v0, ValueN... vn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_amb_from<identity_one_worker, Value0>::value,
            typename    defer_amb_from<identity_one_worker, Value0>::observable_type>::type
        /// \endcond
    {
        return          defer_amb_from<identity_one_worker, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), identity_current_thread());
    }

    /*! For each item from only the first of the given observables deliver from the new observable that is returned, on the specified scheduler.

        \tparam Coordination  the type of the scheduler
        \tparam Value0        ...
        \tparam ValueN        types of source observables

        \param  cn  the scheduler to synchronize sources from different contexts.
        \param  v0  ...
        \param  vn  source observables

        \return  Observable that emits the same sequence as whichever of the source observables first emitted an item or sent a termination notification.

        \sample
        \snippet amb.cpp threaded amb sample
        \snippet output.txt threaded amb sample
    */
    template<class Coordination, class Value0, class... ValueN>
    auto amb(Coordination cn, Value0 v0, ValueN... vn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_amb_from<Coordination, Value0>::value,
            typename    defer_amb_from<Coordination, Value0>::observable_type>::type
        /// \endcond
    {
        return          defer_amb_from<Coordination, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), std::move(cn));
    }

    /*! For each item from this observable use the CollectionSelector to produce an observable and subscribe to that observable.
        For each item from all of the produced observables use the ResultSelector to produce a value to emit from the new observable that is returned.

        \tparam CollectionSelector  the type of the observable producing function
        \tparam ResultSelector      the type of the aggregation function

        \param  s   a function that returns an observable for each item emitted by the source observable
        \param  rs  a function that combines one item emitted by each of the source and collection observables and returns an item to be emitted by the resulting observable

        \return  Observable that emits the results of applying a function to a pair of values emitted by the source observable and the collection observable.

        Observables, produced by the CollectionSelector, are merged. There is another operator rxcpp::observable<T,SourceType>::concat_map that works similar but concatenates the observables.

        \sample
        \snippet flat_map.cpp flat_map sample
        \snippet output.txt flat_map sample
    */
    template<class CollectionSelector, class ResultSelector>
    auto flat_map(CollectionSelector&& s, ResultSelector&& rs) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<rxu::value_type_t<rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>,  rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>
        /// \endcond
    {
        return  observable<rxu::value_type_t<rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>,  rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>(
                                                                                                                                          rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), identity_current_thread()));
    }

    /*! For each item from this observable use the CollectionSelector to produce an observable and subscribe to that observable.
        For each item from all of the produced observables use the ResultSelector to produce a value to emit from the new observable that is returned.

        \tparam CollectionSelector  the type of the observable producing function
        \tparam ResultSelector      the type of the aggregation function
        \tparam Coordination        the type of the scheduler

        \param  s   a function that returns an observable for each item emitted by the source observable
        \param  rs  a function that combines one item emitted by each of the source and collection observables and returns an item to be emitted by the resulting observable
        \param  cn  the scheduler to synchronize sources from different contexts.

        \return  Observable that emits the results of applying a function to a pair of values emitted by the source observable and the collection observable.

        Observables, produced by the CollectionSelector, are merged. There is another operator rxcpp::observable<T,SourceType>::concat_map that works similar but concatenates the observables.

        \sample
        \snippet flat_map.cpp threaded flat_map sample
        \snippet output.txt threaded flat_map sample
    */
    template<class CollectionSelector, class ResultSelector, class Coordination>
    auto flat_map(CollectionSelector&& s, ResultSelector&& rs, Coordination&& cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<rxu::value_type_t<rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>>, rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>>
        /// \endcond
    {
        return  observable<rxu::value_type_t<rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>>, rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>>(
                                                                                                                                  rxo::detail::flat_map<this_type, CollectionSelector, ResultSelector, Coordination>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), std::forward<Coordination>(cn)));
    }

    /// \cond SHOW_SERVICE_MEMBERS
    template<class Coordination>
    struct defer_concat : public defer_observable<
        is_observable<value_type>,
        this_type,
        rxo::detail::concat, value_type, observable<value_type>, Coordination>
    {
    };
    /// \endcond

    /*! For each item from this observable subscribe to one at a time, in the order received.
        For each item from all of the nested observables deliver from the new observable that is returned.

        \return  Observable that emits the items emitted by each of the Observables emitted by the source observable, one after the other, without interleaving them.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet concat.cpp implicit concat sample
        \snippet output.txt implicit concat sample
    */
    template<class... AN>
    auto concat(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename defer_concat<identity_one_worker>::observable_type
        /// \endcond
    {
        return      defer_concat<identity_one_worker>::make(*this, *this, identity_current_thread());
        static_assert(sizeof...(AN) == 0, "concat() was passed too many arguments.");
    }

    /*! For each item from this observable subscribe to one at a time, in the order received.
        For each item from all of the nested observables deliver from the new observable that is returned.

        \tparam  Coordination  the type of the scheduler

        \param  cn  the scheduler to synchronize sources from different contexts.

        \return  Observable that emits the items emitted by each of the Observables emitted by the source observable, one after the other, without interleaving them.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet concat.cpp threaded implicit concat sample
        \snippet output.txt threaded implicit concat sample
    */
    template<class Coordination>
    auto concat(Coordination cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_concat<Coordination>::value,
            typename    defer_concat<Coordination>::observable_type>::type
        /// \endcond
    {
        return          defer_concat<Coordination>::make(*this, *this, std::move(cn));
    }

    /// \cond SHOW_SERVICE_MEMBERS
    template<class Coordination, class Value0>
    struct defer_concat_from : public defer_observable<
        rxu::all_true<
            is_coordination<Coordination>::value,
            is_observable<Value0>::value>,
        this_type,
        rxo::detail::concat, observable<value_type>, observable<observable<value_type>>, Coordination>
    {
    };
    /// \endcond

    /*! For each given observable subscribe to one at a time, in the order received.
        For each emitted item deliver from the new observable that is returned.

        \tparam Value0  ...
        \tparam ValueN  types of source observables

        \param  v0  ...
        \param  vn  source observables

        \return  Observable that emits items emitted by the source observables, one after the other, without interleaving them.

        \sample
        \snippet concat.cpp concat sample
        \snippet output.txt concat sample
    */
    template<class Value0, class... ValueN>
    auto concat(Value0 v0, ValueN... vn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_concat_from<identity_one_worker, Value0>::value,
            typename    defer_concat_from<identity_one_worker, Value0>::observable_type>::type
        /// \endcond
    {
        return          defer_concat_from<identity_one_worker, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), identity_current_thread());
    }

    /*! For each given observable subscribe to one at a time, in the order received.
        For each emitted item deliver from the new observable that is returned.

        \tparam Coordination  the type of the scheduler
        \tparam Value0        ...
        \tparam ValueN        types of source observables

        \param  cn  the scheduler to synchronize sources from different contexts.
        \param  v0  ...
        \param  vn  source observables

        \return  Observable that emits items emitted by the source observables, one after the other, without interleaving them.

        \sample
        \snippet concat.cpp threaded concat sample
        \snippet output.txt threaded concat sample
    */
    template<class Coordination, class Value0, class... ValueN>
    auto concat(Coordination cn, Value0 v0, ValueN... vn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->  typename std::enable_if<
                        defer_concat_from<Coordination, Value0>::value,
            typename    defer_concat_from<Coordination, Value0>::observable_type>::type
        /// \endcond
    {
        return          defer_concat_from<Coordination, Value0>::make(*this, rxs::from(this->as_dynamic(), v0.as_dynamic(), vn.as_dynamic()...), std::move(cn));
    }

    /*! For each item from this observable use the CollectionSelector to produce an observable and subscribe to that observable.
        For each item from all of the produced observables use the ResultSelector to produce a value to emit from the new observable that is returned.

        \tparam CollectionSelector  the type of the observable producing function
        \tparam ResultSelector      the type of the aggregation function

        \param  s   a function that returns an observable for each item emitted by the source observable
        \param  rs  a function that combines one item emitted by each of the source and collection observables and returns an item to be emitted by the resulting observable

        \return  Observable that emits the results of applying a function to a pair of values emitted by the source observable and the collection observable.

        Observables, produced by the CollectionSelector, are concatenated. There is another operator rxcpp::observable<T,SourceType>::flat_map that works similar but merges the observables.

        \sample
        \snippet concat_map.cpp concat_map sample
        \snippet output.txt concat_map sample
    */
    template<class CollectionSelector, class ResultSelector>
    auto concat_map(CollectionSelector&& s, ResultSelector&& rs) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<rxu::value_type_t<rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>,    rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>
        /// \endcond
    {
        return  observable<rxu::value_type_t<rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>,    rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>>(
                                                                                                                                              rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, identity_one_worker>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), identity_current_thread()));
    }

    /*! For each item from this observable use the CollectionSelector to produce an observable and subscribe to that observable.
        For each item from all of the produced observables use the ResultSelector to produce a value to emit from the new observable that is returned.

        \tparam CollectionSelector  the type of the observable producing function
        \tparam ResultSelector      the type of the aggregation function
        \tparam Coordination        the type of the scheduler

        \param  s   a function that returns an observable for each item emitted by the source observable
        \param  rs  a function that combines one item emitted by each of the source and collection observables and returns an item to be emitted by the resulting observable
        \param  cn  the scheduler to synchronize sources from different contexts.

        \return  Observable that emits the results of applying a function to a pair of values emitted by the source observable and the collection observable.

        Observables, produced by the CollectionSelector, are concatenated. There is another operator rxcpp::observable<T,SourceType>::flat_map that works similar but merges the observables.

        \sample
        \snippet concat_map.cpp threaded concat_map sample
        \snippet output.txt threaded concat_map sample
    */
    template<class CollectionSelector, class ResultSelector, class Coordination>
    auto concat_map(CollectionSelector&& s, ResultSelector&& rs, Coordination&& cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<rxu::value_type_t<rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>>,   rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>>
        /// \endcond
    {
        return  observable<rxu::value_type_t<rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>>,   rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>>(
                                                                                                                                      rxo::detail::concat_map<this_type, CollectionSelector, ResultSelector, Coordination>(*this, std::forward<CollectionSelector>(s), std::forward<ResultSelector>(rs), std::forward<Coordination>(cn)));
    }

    /*! @copydoc rx-with_latest_from.hpp
     */
    template<class... AN>
    auto with_latest_from(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(with_latest_from_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(with_latest_from_tag{},                *this, std::forward<AN>(an)...);
    }


    /*! @copydoc rx-combine_latest.hpp
     */
    template<class... AN>
    auto combine_latest(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(combine_latest_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(combine_latest_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-zip.hpp
     */
    template<class... AN>
    auto zip(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(zip_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(zip_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-group_by.hpp
     */
    template<class... AN>
    inline auto group_by(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(group_by_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(group_by_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-ignore_elements.hpp
     */
    template<class... AN>
    auto ignore_elements(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(ignore_elements_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(ignore_elements_tag{},                *this, std::forward<AN>(an)...);
    }

    /// \cond SHOW_SERVICE_MEMBERS
    /// multicast ->
    /// allows connections to the source to be independent of subscriptions
    ///
    template<class Subject>
    auto multicast(Subject sub) const
        ->      connectable_observable<T,   rxo::detail::multicast<T, this_type, Subject>> {
        return  connectable_observable<T,   rxo::detail::multicast<T, this_type, Subject>>(
                                            rxo::detail::multicast<T, this_type, Subject>(*this, std::move(sub)));
    }
    /// \endcond

    /*! Turn a cold observable hot and allow connections to the source to be independent of subscriptions.

        \tparam  Coordination  the type of the scheduler

        \param  cn  a scheduler all values are queued and delivered on
        \param  cs  the subscription to control lifetime

        \return  rxcpp::connectable_observable that upon connection causes the source observable to emit items to its observers, on the specified scheduler.

        \sample
        \snippet publish.cpp publish_synchronized sample
        \snippet output.txt publish_synchronized sample
    */
    template<class Coordination>
    auto publish_synchronized(Coordination cn, composite_subscription cs = composite_subscription()) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::synchronize<T, Coordination>(std::move(cn), cs)))
        /// \endcond
    {
        return                    multicast(rxsub::synchronize<T, Coordination>(std::move(cn), cs));
    }

    /*! Turn a cold observable hot and allow connections to the source to be independent of subscriptions.

        \return  rxcpp::connectable_observable that upon connection causes the source observable to emit items to its observers.

        \sample
        \snippet publish.cpp publish subject sample
        \snippet output.txt publish subject sample
    */
    template<class... AN>
    auto publish(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::subject<T>(composite_subscription())))
        /// \endcond
    {
        composite_subscription cs;
        return                    multicast(rxsub::subject<T>(cs));
        static_assert(sizeof...(AN) == 0, "publish() was passed too many arguments.");
    }

    /*! Turn a cold observable hot and allow connections to the source to be independent of subscriptions.

        \param  cs  the subscription to control lifetime

        \return  rxcpp::connectable_observable that upon connection causes the source observable to emit items to its observers.

        \sample
        \snippet publish.cpp publish subject sample
        \snippet output.txt publish subject sample
    */
    template<class... AN>
    auto publish(composite_subscription cs, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::subject<T>(cs)))
        /// \endcond
    {
        return                    multicast(rxsub::subject<T>(cs));
        static_assert(sizeof...(AN) == 0, "publish(composite_subscription) was passed too many arguments.");
    }

    /*! Turn a cold observable hot, send the most recent value to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \tparam  T  the type of the emitted item

        \param  first  an initial item to be emitted by the resulting observable at connection time before emitting the items from the source observable; not emitted to observers that subscribe after the time of connection

        \return  rxcpp::connectable_observable that upon connection causes the source observable to emit items to its observers.

        \sample
        \snippet publish.cpp publish behavior sample
        \snippet output.txt publish behavior sample
    */
    template<class... AN>
    auto publish(T first, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::behavior<T>(first, composite_subscription())))
        /// \endcond
    {
        composite_subscription cs;
        return      multicast(rxsub::behavior<T>(first, cs));
        static_assert(sizeof...(AN) == 0, "publish(value_type) was passed too many arguments.");
    }

    /*! Turn a cold observable hot, send the most recent value to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \tparam  T  the type of the emitted item

        \param  first  an initial item to be emitted by the resulting observable at connection time before emitting the items from the source observable; not emitted to observers that subscribe after the time of connection
        \param  cs     the subscription to control lifetime

        \return  rxcpp::connectable_observable that upon connection causes the source observable to emit items to its observers.

        \sample
        \snippet publish.cpp publish behavior sample
        \snippet output.txt publish behavior sample
    */
    template<class... AN>
    auto publish(T first, composite_subscription cs, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::behavior<T>(first, cs)))
        /// \endcond
    {
        return      multicast(rxsub::behavior<T>(first, cs));
        static_assert(sizeof...(AN) == 0, "publish(value_type, composite_subscription) was passed too many arguments.");
    }

    /*! Turn a cold observable hot, send all earlier emitted values to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay all of its items and notifications to any future observer.

        \sample
        \snippet replay.cpp replay sample
        \snippet output.txt replay sample
    */
    template<class... AN>
    auto replay(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, identity_one_worker>(identity_current_thread(), composite_subscription())))
        /// \endcond
    {
        composite_subscription cs;
        return      multicast(rxsub::replay<T, identity_one_worker>(identity_current_thread(), cs));
        static_assert(sizeof...(AN) == 0, "replay() was passed too many arguments.");
    }

    /*! Turn a cold observable hot, send all earlier emitted values to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \param  cs  the subscription to control lifetime

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay all of its items and notifications to any future observer.

        \sample
        \snippet replay.cpp replay sample
        \snippet output.txt replay sample
    */
    template<class... AN>
    auto replay(composite_subscription cs, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, identity_one_worker>(identity_current_thread(), cs)))
        /// \endcond
    {
        return      multicast(rxsub::replay<T, identity_one_worker>(identity_current_thread(), cs));
        static_assert(sizeof...(AN) == 0, "replay(composite_subscription) was passed too many arguments.");
    }

    /*! Turn a cold observable hot, send all earlier emitted values to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \tparam  Coordination  the type of the scheduler

        \param  cn  a scheduler all values are queued and delivered on
        \param  cs  the subscription to control lifetime

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay all of its items and notifications to any future observer.

        \sample
        \snippet replay.cpp threaded replay sample
        \snippet output.txt threaded replay sample
    */
    template<class Coordination,
        class Requires = typename std::enable_if<is_coordination<Coordination>::value, rxu::types_checked>::type>
    auto replay(Coordination cn, composite_subscription cs = composite_subscription()) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, Coordination>(std::move(cn), cs)))
        /// \endcond
    {
        return      multicast(rxsub::replay<T, Coordination>(std::move(cn), cs));
    }

    /*! Turn a cold observable hot, send at most count of earlier emitted values to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \param  count  the maximum number of the most recent items sent to new observers

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay at most count items to any future observer.

        \sample
        \snippet replay.cpp replay count sample
        \snippet output.txt replay count sample
    */
    template<class... AN>
    auto replay(std::size_t count, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, identity_one_worker>(count, identity_current_thread(), composite_subscription())))
        /// \endcond
    {
        composite_subscription cs;
        return      multicast(rxsub::replay<T, identity_one_worker>(count, identity_current_thread(), cs));
        static_assert(sizeof...(AN) == 0, "replay(count) was passed too many arguments.");
    }

    /*! Turn a cold observable hot, send at most count of earlier emitted values to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \param  count  the maximum number of the most recent items sent to new observers
        \param  cs     the subscription to control lifetime

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay at most count items to any future observer.

        \sample
        \snippet replay.cpp replay count sample
        \snippet output.txt replay count sample
    */
    template<class... AN>
    auto replay(std::size_t count, composite_subscription cs, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, identity_one_worker>(count, identity_current_thread(), cs)))
        /// \endcond
    {
        return      multicast(rxsub::replay<T, identity_one_worker>(count, identity_current_thread(), cs));
        static_assert(sizeof...(AN) == 0, "replay(count, composite_subscription) was passed too many arguments.");
    }

    /*! Turn a cold observable hot, send at most count of earlier emitted values to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \tparam  Coordination  the type of the scheduler

        \param  count  the maximum number of the most recent items sent to new observers
        \param  cn     a scheduler all values are queued and delivered on
        \param  cs     the subscription to control lifetime

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay at most count items to any future observer.

        \sample
        \snippet replay.cpp threaded replay count sample
        \snippet output.txt threaded replay count sample
    */
    template<class Coordination,
        class Requires = typename std::enable_if<is_coordination<Coordination>::value, rxu::types_checked>::type>
    auto replay(std::size_t count, Coordination cn, composite_subscription cs = composite_subscription()) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, Coordination>(count, std::move(cn), cs)))
        /// \endcond
    {
        return      multicast(rxsub::replay<T, Coordination>(count, std::move(cn), cs));
    }

    /*! Turn a cold observable hot, send values emitted within a specified time window to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \param  period  the duration of the window in which the replayed items must be emitted
        \param  cs      the subscription to control lifetime

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay items emitted within a specified time window to any future observer.

        \sample
        \snippet replay.cpp replay period sample
        \snippet output.txt replay period sample
    */
    template<class Duration>
    auto replay(Duration period, composite_subscription cs = composite_subscription()) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, identity_one_worker>(period, identity_current_thread(), cs)))
        /// \endcond
    {
        return      multicast(rxsub::replay<T, identity_one_worker>(period, identity_current_thread(), cs));
    }

    /*! Turn a cold observable hot, send values emitted within a specified time window to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \tparam  Coordination  the type of the scheduler

        \param  period  the duration of the window in which the replayed items must be emitted
        \param  cn      a scheduler all values are queued and delivered on
        \param  cs      the subscription to control lifetime

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay items emitted within a specified time window to any future observer.

        \sample
        \snippet replay.cpp threaded replay period sample
        \snippet output.txt threaded replay period sample
    */
    template<class Coordination,
        class Requires = typename std::enable_if<is_coordination<Coordination>::value, rxu::types_checked>::type>
    auto replay(rxsc::scheduler::clock_type::duration period, Coordination cn, composite_subscription cs = composite_subscription()) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, Coordination>(period, std::move(cn), cs)))
        /// \endcond
    {
        return      multicast(rxsub::replay<T, Coordination>(period, std::move(cn), cs));
    }

    /*! Turn a cold observable hot, send at most count of values emitted within a specified time window to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \param  count   the maximum number of the most recent items sent to new observers
        \param  period  the duration of the window in which the replayed items must be emitted
        \param  cs      the subscription to control lifetime

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay at most count of items emitted within a specified time window to any future observer.

        \sample
        \snippet replay.cpp replay count+period sample
        \snippet output.txt replay count+period sample
    */
    template<class Duration>
    auto replay(std::size_t count, Duration period, composite_subscription cs = composite_subscription()) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, identity_one_worker>(count, period, identity_current_thread(), cs)))
        /// \endcond
    {
        return      multicast(rxsub::replay<T, identity_one_worker>(count, period, identity_current_thread(), cs));
    }

    /*! Turn a cold observable hot, send at most count of values emitted within a specified time window to any new subscriber, and allow connections to the source to be independent of subscriptions.

        \tparam  Coordination  the type of the scheduler

        \param  count   the maximum number of the most recent items sent to new observers
        \param  period  the duration of the window in which the replayed items must be emitted
        \param  cn      a scheduler all values are queued and delivered on
        \param  cs      the subscription to control lifetime

        \return  rxcpp::connectable_observable that shares a single subscription to the underlying observable that will replay at most count of items emitted within a specified time window to any future observer.

        \sample
        \snippet replay.cpp threaded replay count+period sample
        \snippet output.txt threaded replay count+period sample
    */
    template<class Coordination,
        class Requires = typename std::enable_if<is_coordination<Coordination>::value, rxu::types_checked>::type>
    auto replay(std::size_t count, rxsc::scheduler::clock_type::duration period, Coordination cn, composite_subscription cs = composite_subscription()) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS multicast(rxsub::replay<T, Coordination>(count, period, std::move(cn), cs)))
        /// \endcond
    {
        return      multicast(rxsub::replay<T, Coordination>(count, period, std::move(cn), cs));
    }

    /*! Subscription and unsubscription are queued and delivered using the scheduler from the supplied coordination.

        \tparam Coordination  the type of the scheduler

        \param  cn  the scheduler to perform subscription actions on

        \return  The source observable modified so that its subscriptions happen on the specified scheduler.

        \sample
        \snippet subscribe_on.cpp subscribe_on sample
        \snippet output.txt subscribe_on sample

        Invoking rxcpp::observable::observe_on operator, instead of subscribe_on, gives following results:
        \snippet output.txt observe_on sample
    */
    template<class Coordination>
    auto subscribe_on(Coordination cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<rxu::value_type_t<rxo::detail::subscribe_on<T, this_type, Coordination>>,  rxo::detail::subscribe_on<T, this_type, Coordination>>
        /// \endcond
    {
        return  observable<rxu::value_type_t<rxo::detail::subscribe_on<T, this_type, Coordination>>,  rxo::detail::subscribe_on<T, this_type, Coordination>>(
                                                                                                      rxo::detail::subscribe_on<T, this_type, Coordination>(*this, std::move(cn)));
    }

    /*! All values are queued and delivered using the scheduler from the supplied coordination.

        \tparam Coordination  the type of the scheduler

        \param  cn  the scheduler to notify observers on

        \return  The source observable modified so that its observers are notified on the specified scheduler.

        \sample
        \snippet observe_on.cpp observe_on sample
        \snippet output.txt observe_on sample

        Invoking rxcpp::observable::subscribe_on operator, instead of observe_on, gives following results:
        \snippet output.txt subscribe_on sample
    */
    template<class Coordination>
    auto observe_on(Coordination cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::observe_on<T, Coordination>(std::move(cn))))
        /// \endcond
    {
        return                    lift<T>(rxo::detail::observe_on<T, Coordination>(std::move(cn)));
    }

    /*! @copydoc rx-reduce.hpp
     */
    template<class... AN>
    auto reduce(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(reduce_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(reduce_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rxcpp::operators::first
     */
    template<class... AN>
    auto first(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(delayed_type<first_tag, AN...>::value(), *(this_type*)nullptr))
        /// \endcond
    {
        return      observable_member(delayed_type<first_tag, AN...>::value(),                *this);
        static_assert(sizeof...(AN) == 0, "first() was passed too many arguments.");
    }

    /*! @copydoc rxcpp::operators::last
     */
    template<class... AN>
    auto last(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(delayed_type<last_tag, AN...>::value(), *(this_type*)nullptr))
        /// \endcond
    {
        return      observable_member(delayed_type<last_tag, AN...>::value(),                *this);
        static_assert(sizeof...(AN) == 0, "last() was passed too many arguments.");
    }

    /*! @copydoc rxcpp::operators::count
     */
    template<class... AN>
    auto count(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(delayed_type<reduce_tag, AN...>::value(), *(this_type*)nullptr, 0, rxu::count(), identity_for<int>()))
        /// \endcond
    {
        return      observable_member(delayed_type<reduce_tag, AN...>::value(),                *this, 0, rxu::count(), identity_for<int>());
        static_assert(sizeof...(AN) == 0, "count() was passed too many arguments.");
    }

    /*! @copydoc rxcpp::operators::sum
     */
    template<class... AN>
    auto sum(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(delayed_type<sum_tag, AN...>::value(), *(this_type*)nullptr))
        /// \endcond
    {
        return      observable_member(delayed_type<sum_tag, AN...>::value(),                *this);
        static_assert(sizeof...(AN) == 0, "sum() was passed too many arguments.");
    }

    /*! @copydoc rxcpp::operators::average
     */
    template<class... AN>
    auto average(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(delayed_type<average_tag, AN...>::value(), *(this_type*)nullptr))
        /// \endcond
    {
        return      observable_member(delayed_type<average_tag, AN...>::value(),                *this);
        static_assert(sizeof...(AN) == 0, "average() was passed too many arguments.");
    }

    /*! @copydoc rxcpp::operators::max
     */
    template<class... AN>
    auto max(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(delayed_type<max_tag, AN...>::value(), *(this_type*)nullptr))
        /// \endcond
    {
        return      observable_member(delayed_type<max_tag, AN...>::value(),                *this);
        static_assert(sizeof...(AN) == 0, "max() was passed too many arguments.");
    }

    /*! @copydoc rxcpp::operators::min
     */
    template<class... AN>
    auto min(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(delayed_type<min_tag, AN...>::value(), *(this_type*)nullptr))
        /// \endcond
    {
        return      observable_member(delayed_type<min_tag, AN...>::value(),                *this);
        static_assert(sizeof...(AN) == 0, "min() was passed too many arguments.");
    }

    /*! For each item from this observable use Accumulator to combine items into a value that will be emitted from the new observable that is returned.

        \tparam Seed         the type of the initial value for the accumulator
        \tparam Accumulator  the type of the data accumulating function

        \param seed  the initial value for the accumulator
        \param a     an accumulator function to be invoked on each item emitted by the source observable, whose result will be emitted and used in the next accumulator call

        \return  An observable that emits the results of each call to the accumulator function.

        \sample
        \snippet scan.cpp scan sample
        \snippet output.txt scan sample
    */
    template<class Seed, class Accumulator>
    auto scan(Seed seed, Accumulator&& a) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<Seed,    rxo::detail::scan<T, this_type, Accumulator, Seed>>
        /// \endcond
    {
        return  observable<Seed,    rxo::detail::scan<T, this_type, Accumulator, Seed>>(
                                    rxo::detail::scan<T, this_type, Accumulator, Seed>(*this, std::forward<Accumulator>(a), seed));
    }

    /*! Return an Observable that emits the most recent items emitted by the source Observable within periodic time intervals.

        \param period  the period of time to sample the source observable.
        \param coordination  the scheduler for the items.

        \return  Observable that emits the most recently emitted item since the previous sampling.

        \sample
        \snippet sample.cpp sample period sample
        \snippet output.txt sample period sample
    */
    template<class Coordination,
        class Requires = typename std::enable_if<is_coordination<Coordination>::value, rxu::types_checked>::type>
    auto sample_with_time(rxsc::scheduler::clock_type::duration period, Coordination coordination) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::sample_with_time<T, rxsc::scheduler::clock_type::duration, Coordination>(period, coordination)))
        /// \endcond
    {
        return                    lift<T>(rxo::detail::sample_with_time<T, rxsc::scheduler::clock_type::duration, Coordination>(period, coordination));
    }

    /*! Return an Observable that emits the most recent items emitted by the source Observable within periodic time intervals.

        \param period  the period of time to sample the source observable.

        \return  Observable that emits the most recently emitted item since the previous sampling.

        \sample
        \snippet sample.cpp sample period sample
        \snippet output.txt sample period sample
    */
    template<class... AN>
    auto sample_with_time(rxsc::scheduler::clock_type::duration period, AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<T>(rxo::detail::sample_with_time<T, rxsc::scheduler::clock_type::duration, identity_one_worker>(period, identity_current_thread())))
        /// \endcond
    {
        return                    lift<T>(rxo::detail::sample_with_time<T, rxsc::scheduler::clock_type::duration, identity_one_worker>(period, identity_current_thread()));
        static_assert(sizeof...(AN) == 0, "sample_with_time(period) was passed too many arguments.");
    }

    /*! Make new observable with skipped first count items from this observable.

        \tparam  Count  the type of the items counter

        \param  t  the number of items to skip

        \return  An observable that is identical to the source observable except that it does not emit the first t items that the source observable emits.

        \sample
        \snippet skip.cpp skip sample
        \snippet output.txt skip sample
    */
    template<class Count>
    auto skip(Count t) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<T,   rxo::detail::skip<T, this_type, Count>>
        /// \endcond
    {
        return  observable<T,   rxo::detail::skip<T, this_type, Count>>(
                                rxo::detail::skip<T, this_type, Count>(*this, t));
    }

    /*! Make new observable with skipped last count items from this observable.

        \tparam  Count  the type of the items counter

        \param  t  the number of last items to skip

        \return  An observable that is identical to the source observable except that it does not emit the last t items that the source observable emits.

        \sample
        \snippet skip_last.cpp skip_last sample
        \snippet output.txt skip_last sample
    */
    template<class Count>
    auto skip_last(Count t) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<T,   rxo::detail::skip_last<T, this_type, Count>>
        /// \endcond
    {
        return  observable<T,   rxo::detail::skip_last<T, this_type, Count>>(
                                rxo::detail::skip_last<T, this_type, Count>(*this, t));
    }

    /*! Make new observable with items skipped until on_next occurs on the trigger observable

        \tparam  TriggerSource  the type of the trigger observable

        \param  t  an observable that has to emit an item before the source observable's elements begin to be mirrored by the resulting observable

        \return  An observable that skips items from the source observable until the second observable emits an item, then emits the remaining items.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet skip_until.cpp skip_until sample
        \snippet output.txt skip_until sample
    */
    template<class TriggerSource>
    auto skip_until(TriggerSource&& t) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename std::enable_if<is_observable<TriggerSource>::value,
                observable<T,   rxo::detail::skip_until<T, this_type, TriggerSource, identity_one_worker>>>::type
        /// \endcond
    {
        return  observable<T,   rxo::detail::skip_until<T, this_type, TriggerSource, identity_one_worker>>(
                                rxo::detail::skip_until<T, this_type, TriggerSource, identity_one_worker>(*this, std::forward<TriggerSource>(t), identity_one_worker(rxsc::make_current_thread())));
    }

    /*! Make new observable with items skipped until on_next occurs on the trigger observable

        \tparam  TriggerSource  the type of the trigger observable
        \tparam  Coordination   the type of the scheduler

        \param  t   an observable that has to emit an item before the source observable's elements begin to be mirrored by the resulting observable
        \param  cn  the scheduler to use for scheduling the items

        \return  An observable that skips items from the source observable until the second observable emits an item, then emits the remaining items.

        \sample
        \snippet skip_until.cpp threaded skip_until sample
        \snippet output.txt threaded skip_until sample
    */
    template<class TriggerSource, class Coordination>
    auto skip_until(TriggerSource&& t, Coordination&& cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename std::enable_if<is_observable<TriggerSource>::value && is_coordination<Coordination>::value,
                observable<T,   rxo::detail::skip_until<T, this_type, TriggerSource, Coordination>>>::type
        /// \endcond
    {
        return  observable<T,   rxo::detail::skip_until<T, this_type, TriggerSource, Coordination>>(
                                rxo::detail::skip_until<T, this_type, TriggerSource, Coordination>(*this, std::forward<TriggerSource>(t), std::forward<Coordination>(cn)));
    }

    /*! For the first count items from this observable emit them from the new observable that is returned.

        \tparam Count  the type of the items counter

        \param t  the number of items to take

        \return  An observable that emits only the first t items emitted by the source Observable, or all of the items from the source observable if that observable emits fewer than t items.

        \sample
        \snippet take.cpp take sample
        \snippet output.txt take sample
    */
    template<class Count>
    auto take(Count t) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<T,   rxo::detail::take<T, this_type, Count>>
        /// \endcond
    {
        return  observable<T,   rxo::detail::take<T, this_type, Count>>(
                                rxo::detail::take<T, this_type, Count>(*this, t));
    }

    /*! Emit only the final t items emitted by the source Observable.

        \tparam Count  the type of the items counter

        \param t  the number of last items to take

        \return  An observable that emits only the last t items emitted by the source Observable, or all of the items from the source observable if that observable emits fewer than t items.

        \sample
        \snippet take_last.cpp take_last sample
        \snippet output.txt take_last sample
    */
    template<class Count>
    auto take_last(Count t) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<T,   rxo::detail::take_last<T, this_type, Count>>
        /// \endcond
    {
        return  observable<T,   rxo::detail::take_last<T, this_type, Count>>(
                                rxo::detail::take_last<T, this_type, Count>(*this, t));
    }


    /*! For each item from this observable until on_next occurs on the trigger observable, emit them from the new observable that is returned.

        \tparam  TriggerSource  the type of the trigger observable

        \param  t  an observable whose first emitted item will stop emitting items from the source observable

        \return  An observable that emits the items emitted by the source observable until such time as other emits its first item.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet take_until.cpp take_until sample
        \snippet output.txt take_until sample
    */
    template<class TriggerSource>
    auto take_until(TriggerSource t) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename std::enable_if<is_observable<TriggerSource>::value,
                observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>>>::type
        /// \endcond
    {
        return  observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>>(
                                rxo::detail::take_until<T, this_type, TriggerSource, identity_one_worker>(*this, std::move(t), identity_current_thread()));
    }

    /*! For each item from this observable until on_next occurs on the trigger observable, emit them from the new observable that is returned.

        \tparam  TriggerSource  the type of the trigger observable
        \tparam  Coordination   the type of the scheduler

        \param  t   an observable whose first emitted item will stop emitting items from the source observable
        \param  cn  the scheduler to use for scheduling the items

        \return  An observable that emits the items emitted by the source observable until such time as other emits its first item.

        \sample
        \snippet take_until.cpp threaded take_until sample
        \snippet output.txt threaded take_until sample
    */
    template<class TriggerSource, class Coordination>
    auto take_until(TriggerSource t, Coordination cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename std::enable_if<is_observable<TriggerSource>::value && is_coordination<Coordination>::value,
                observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, Coordination>>>::type
        /// \endcond
    {
        return  observable<T,   rxo::detail::take_until<T, this_type, TriggerSource, Coordination>>(
                                rxo::detail::take_until<T, this_type, TriggerSource, Coordination>(*this, std::move(t), std::move(cn)));
    }

    /*! For each item from this observable until the specified time, emit them from the new observable that is returned.

        \tparam  TimePoint  the type of the time interval

        \param  when  an observable whose first emitted item will stop emitting items from the source observable

        \return  An observable that emits those items emitted by the source observable before the time runs out.

        \note All sources must be synchronized! This means that calls across all the subscribers must be serial.

        \sample
        \snippet take_until.cpp take_until time sample
        \snippet output.txt take_until time sample
    */
    template<class TimePoint>
    auto take_until(TimePoint when) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename std::enable_if<std::is_convertible<TimePoint, rxsc::scheduler::clock_type::time_point>::value,
                observable<T,   rxo::detail::take_until<T, this_type, decltype(rxs::timer(when, identity_current_thread())), identity_one_worker>>>::type
        /// \endcond
    {
        auto cn = identity_current_thread();
        return  take_until(rxs::timer(when, cn), cn);
    }

    /*! For each item from this observable until the specified time, emit them from the new observable that is returned.

        \tparam  TimePoint     the type of the time interval
        \tparam  Coordination  the type of the scheduler

        \param  when  an observable whose first emitted item will stop emitting items from the source observable
        \param  cn    the scheduler to use for scheduling the items

        \return  An observable that emits those items emitted by the source observable before the time runs out.

        \sample
        \snippet take_until.cpp threaded take_until time sample
        \snippet output.txt threaded take_until time sample
    */
    template<class Coordination>
    auto take_until(rxsc::scheduler::clock_type::time_point when, Coordination cn) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> typename std::enable_if<is_coordination<Coordination>::value,
                observable<T,   rxo::detail::take_until<T, this_type, decltype(rxs::timer(when, cn)), Coordination>>>::type
        /// \endcond
    {
        return  take_until(rxs::timer(when, cn), cn);
    }

    /*! Infinitely repeat this observable.

        \return  An observable that emits the items emitted by the source observable repeatedly and in sequence.

        \sample
        \snippet repeat.cpp repeat sample
        \snippet output.txt repeat sample

        If the source observable calls on_error, repeat stops:
        \snippet repeat.cpp repeat error sample
        \snippet output.txt repeat error sample
    */
    template<class... AN>
    auto repeat(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<T, rxo::detail::repeat<T, this_type, int>>
        /// \endcond
    {
        return  observable<T, rxo::detail::repeat<T, this_type, int>>(
            rxo::detail::repeat<T, this_type, int>(*this, 0));
        static_assert(sizeof...(AN) == 0, "repeat() was passed too many arguments.");
    }

    /*! Repeat this observable for the given number of times.

        \tparam Count  the type of the counter

        \param t  the number of times the source observable items are repeated

        \return  An observable that repeats the sequence of items emitted by the source observable for t times.

        Call to repeat(0) infinitely repeats the source observable.

        \sample
        \snippet repeat.cpp repeat count sample
        \snippet output.txt repeat count sample
    */
    template<class Count>
    auto repeat(Count t) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<T, rxo::detail::repeat<T, this_type, Count>>
        /// \endcond
    {
        return  observable<T, rxo::detail::repeat<T, this_type, Count>>(
            rxo::detail::repeat<T, this_type, Count>(*this, t));
    }

    /*! Infinitely retry this observable.

        \return  An observable that mirrors the source observable, resubscribing to it if it calls on_error.

        \sample
        \snippet retry.cpp retry sample
        \snippet output.txt retry sample
    */
    template<class... AN>
    auto retry(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<T, rxo::detail::retry<T, this_type, int>>
        /// \endcond
    {
        return  observable<T, rxo::detail::retry<T, this_type, int>>(
            rxo::detail::retry<T, this_type, int>(*this, 0));
        static_assert(sizeof...(AN) == 0, "retry() was passed too many arguments.");
    }

    /*! Retry this observable for the given number of times.

        \tparam Count  the type of the counter

        \param t  the number of retries

        \return  An observable that mirrors the source observable, resubscribing to it if it calls on_error up to a specified number of retries.

        Call to retry(0) infinitely retries the source observable.

        \sample
        \snippet retry.cpp retry count sample
        \snippet output.txt retry count sample
    */
    template<class Count>
    auto retry(Count t) const
        /// \cond SHOW_SERVICE_MEMBERS
        ->      observable<T, rxo::detail::retry<T, this_type, Count>>
        /// \endcond
    {
        return  observable<T, rxo::detail::retry<T, this_type, Count>>(
            rxo::detail::retry<T, this_type, Count>(*this, t));
    }

    /*! Start with the supplied values, then concatenate this observable.

        \tparam Value0      ...
        \tparam ValueN      the type of sending values

        \param  v0  ...
        \param  vn  values to send

        \return  Observable that emits the specified items and then emits the items emitted by the source observable.

        \sample
        \snippet start_with.cpp short start_with sample
        \snippet output.txt short start_with sample

        Another form of this operator, rxcpp::observable<void, void>::start_with, gets the source observable as a parameter:
        \snippet start_with.cpp full start_with sample
        \snippet output.txt full start_with sample
    */
    template<class Value0, class... ValueN>
    auto start_with(Value0 v0, ValueN... vn) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(rxo::start_with(std::move(v0), std::move(vn)...)(*(this_type*)nullptr))
        /// \endcond
    {
        return      rxo::start_with(std::move(v0), std::move(vn)...)(*this);
    }

    /*! Take values pairwise from this observable.

        \return  Observable that emits tuples of two the most recent items emitted by the source observable.

        \sample
        \snippet pairwise.cpp pairwise sample
        \snippet output.txt pairwise sample

        If the source observable emits less than two items, no pairs are emitted  by the source observable:
        \snippet pairwise.cpp pairwise short sample
        \snippet output.txt pairwise short sample
    */
    template<class... AN>
    auto pairwise(AN**...) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(EXPLICIT_THIS lift<rxu::value_type_t<rxo::detail::pairwise<T>>>(rxo::detail::pairwise<T>()))
        /// \endcond
    {
        return                    lift<rxu::value_type_t<rxo::detail::pairwise<T>>>(rxo::detail::pairwise<T>());
        static_assert(sizeof...(AN) == 0, "pairwise() was passed too many arguments.");
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

/*!
    \defgroup group-core Basics

    \brief These are the core classes that combine to represent a set of values emitted over time that can be cancelled.

    \class rxcpp::observable<void, void>

    \brief typed as ```rxcpp::observable<>```, this is a collection of factory methods that return an observable.

    \ingroup group-core

    \par Create a new type of observable

    \sample
    \snippet create.cpp Create sample
    \snippet output.txt Create sample

    \par Create an observable that emits a range of values

    \sample
    \snippet range.cpp range sample
    \snippet output.txt range sample

    \par Create an observable that emits nothing / generates an error / immediately completes

    \sample
    \snippet never.cpp never sample
    \snippet output.txt never sample
    \snippet error.cpp error sample
    \snippet output.txt error sample
    \snippet empty.cpp empty sample
    \snippet output.txt empty sample

    \par Create an observable that generates new observable for each subscriber

    \sample
    \snippet defer.cpp defer sample
    \snippet output.txt defer sample

    \par Create an observable that emits items every specified interval of time

    \sample
    \snippet interval.cpp interval sample
    \snippet output.txt interval sample

    \par Create an observable that emits items in the specified interval of time

    \sample
    \snippet timer.cpp duration timer sample
    \snippet output.txt duration timer sample

    \par Create an observable that emits all items from a collection

    \sample
    \snippet iterate.cpp iterate sample
    \snippet output.txt iterate sample

    \par Create an observable that emits a set of specified items

    \sample
    \snippet from.cpp from sample
    \snippet output.txt from sample

    \par Create an observable that emits a single item

    \sample
    \snippet just.cpp just sample
    \snippet output.txt just sample

    \par Create an observable that emits a set of items and then subscribes to another observable

    \sample
    \snippet start_with.cpp full start_with sample
    \snippet output.txt full start_with sample

    \par Create an observable that generates a new observable based on a generated resource for each subscriber

    \sample
    \snippet scope.cpp scope sample
    \snippet output.txt scope sample

*/
template<>
class observable<void, void>
{
    ~observable();
public:
    /*! Returns an observable that executes the specified function when a subscriber subscribes to it.

        \tparam T  the type of the items that this observable emits
        \tparam OnSubscribe  the type of OnSubscribe handler function

        \param  os  OnSubscribe event handler

        \return  Observable that executes the specified function when a Subscriber subscribes to it.

        \sample
        \snippet create.cpp Create sample
        \snippet output.txt Create sample

        \warning
        It is good practice to check the observer's is_subscribed state from within the function you pass to create
        so that your observable can stop emitting items or doing expensive calculations when there is no longer an interested observer.

        \badcode
        \snippet create.cpp Create bad code
        \snippet output.txt Create bad code

        \goodcode
        \snippet create.cpp Create good code
        \snippet output.txt Create good code

        \warning
        It is good practice to use operators like observable::take to control lifetime rather than use the subscription explicitly.

        \goodcode
        \snippet create.cpp Create great code
        \snippet output.txt Create great code
    */
    template<class T, class OnSubscribe>
    static auto create(OnSubscribe os)
        -> decltype(rxs::create<T>(std::move(os))) {
        return      rxs::create<T>(std::move(os));
    }
    /*! Returns an observable that sends values in the range first-last by adding step to the previous value.

        \tparam T  the type of the values that this observable emits

        \param  first  first value to send
        \param  last   last value to send
        \param  step   value to add to the previous value to get the next value

        \return  Observable that sends values in the range first-last by adding step to the previous value.

        \sample
        \snippet range.cpp range sample
        \snippet output.txt range sample
    */
    template<class T>
    static auto range(T first = 0, T last = std::numeric_limits<T>::max(), std::ptrdiff_t step = 1)
        -> decltype(rxs::range<T>(first, last, step, identity_current_thread())) {
        return      rxs::range<T>(first, last, step, identity_current_thread());
    }
    /*! Returns an observable that sends values in the range ```first```-```last``` by adding ```step``` to the previous value. The values are sent on the specified scheduler.

        \tparam T             the type of the values that this observable emits
        \tparam Coordination  the type of the scheduler

        \param  first  first value to send
        \param  last   last value to send
        \param  step   value to add to the previous value to get the next value
        \param  cn     the scheduler to run the generator loop on

        \return  Observable that sends values in the range first-last by adding step to the previous value using the specified scheduler.

        \note  `step` or both `step` & `last` may be omitted.

        \sample
        \snippet range.cpp threaded range sample
        \snippet output.txt threaded range sample

        An alternative way to specify the scheduler for emitted values is to use observable::subscribe_on operator
        \snippet range.cpp subscribe_on range sample
        \snippet output.txt subscribe_on range sample
    */
    template<class T, class Coordination>
    static auto range(T first, T last, std::ptrdiff_t step, Coordination cn)
        -> decltype(rxs::range<T>(first, last, step, std::move(cn))) {
        return      rxs::range<T>(first, last, step, std::move(cn));
    }
    /// Returns an observable that sends values in the range ```first```-```last``` by adding 1 to the previous value. The values are sent on the specified scheduler.
    ///
    /// \see       rxcpp::observable<void,void>#range(T first, T last, std::ptrdiff_t step, Coordination cn)
    template<class T, class Coordination>
    static auto range(T first, T last, Coordination cn)
        -> decltype(rxs::range<T>(first, last, std::move(cn))) {
        return      rxs::range<T>(first, last, std::move(cn));
    }
    /// Returns an observable that infinitely (until overflow) sends values starting from ```first```. The values are sent on the specified scheduler.
    ///
    /// \see       rxcpp::observable<void,void>#range(T first, T last, std::ptrdiff_t step, Coordination cn)
    template<class T, class Coordination>
    static auto range(T first, Coordination cn)
        -> decltype(rxs::range<T>(first, std::move(cn))) {
        return      rxs::range<T>(first, std::move(cn));
    }
    /*! Returns an observable that never sends any items or notifications to observer.

        \tparam T  the type of (not) emitted items

        \return  Observable that never sends any items or notifications to observer.

        \sample
        \snippet never.cpp never sample
        \snippet output.txt never sample
    */
    template<class T>
    static auto never()
        -> decltype(rxs::never<T>()) {
        return      rxs::never<T>();
    }
    /*! Returns an observable that calls the specified observable factory to create an observable for each new observer that subscribes.

        \tparam ObservableFactory  the type of the observable factory

        \param  of  the observable factory function to invoke for each observer that subscribes to the resulting observable

        \return  observable whose observers' subscriptions trigger an invocation of the given observable factory function

        \sample
        \snippet defer.cpp defer sample
        \snippet output.txt defer sample
    */
    template<class ObservableFactory>
    static auto defer(ObservableFactory of)
        -> decltype(rxs::defer(std::move(of))) {
        return      rxs::defer(std::move(of));
    }
    /*! Returns an observable that emits a sequential integer every specified time interval.

        \param  period   period between emitted values

        \return  Observable that sends a sequential integer each time interval

        \sample
        \snippet interval.cpp immediate interval sample
        \snippet output.txt immediate interval sample
    */
    template<class... AN>
    static auto interval(rxsc::scheduler::clock_type::duration period, AN**...)
        -> decltype(rxs::interval(period)) {
        return      rxs::interval(period);
        static_assert(sizeof...(AN) == 0, "interval(period) was passed too many arguments.");
    }
    /*! Returns an observable that emits a sequential integer every specified time interval, on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param  period   period between emitted values
        \param  cn       the scheduler to use for scheduling the items

        \return  Observable that sends a sequential integer each time interval

        \sample
        \snippet interval.cpp threaded immediate interval sample
        \snippet output.txt threaded immediate interval sample
    */
    template<class Coordination>
    static auto interval(rxsc::scheduler::clock_type::duration period, Coordination cn)
        -> decltype(rxs::interval(period, std::move(cn))) {
        return      rxs::interval(period, std::move(cn));
    }
    /*! Returns an observable that emits a sequential integer every specified time interval starting from the specified time point.

        \param  initial  time when the first value is sent
        \param  period   period between emitted values

        \return  Observable that sends a sequential integer each time interval

        \sample
        \snippet interval.cpp interval sample
        \snippet output.txt interval sample
    */
    template<class... AN>
    static auto interval(rxsc::scheduler::clock_type::time_point initial, rxsc::scheduler::clock_type::duration period, AN**...)
        -> decltype(rxs::interval(initial, period)) {
        return      rxs::interval(initial, period);
        static_assert(sizeof...(AN) == 0, "interval(initial, period) was passed too many arguments.");
    }
    /*! Returns an observable that emits a sequential integer every specified time interval starting from the specified time point, on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param  initial  time when the first value is sent
        \param  period   period between emitted values
        \param  cn       the scheduler to use for scheduling the items

        \return  Observable that sends a sequential integer each time interval

        \sample
        \snippet interval.cpp threaded interval sample
        \snippet output.txt threaded interval sample
    */
    template<class Coordination>
    static auto interval(rxsc::scheduler::clock_type::time_point initial, rxsc::scheduler::clock_type::duration period, Coordination cn)
        -> decltype(rxs::interval(initial, period, std::move(cn))) {
        return      rxs::interval(initial, period, std::move(cn));
    }
    /*! Returns an observable that emits an integer at the specified time point.

        \param  when  time point when the value is emitted

        \return  Observable that emits an integer at the specified time point

        \sample
        \snippet timer.cpp timepoint timer sample
        \snippet output.txt timepoint timer sample
    */
    template<class... AN>
    static auto timer(rxsc::scheduler::clock_type::time_point at, AN**...)
        -> decltype(rxs::timer(at)) {
        return      rxs::timer(at);
        static_assert(sizeof...(AN) == 0, "timer(at) was passed too many arguments.");
    }
    /*! Returns an observable that emits an integer in the specified time interval.

        \param  when  interval when the value is emitted

        \return  Observable that emits an integer in the specified time interval

        \sample
        \snippet timer.cpp duration timer sample
        \snippet output.txt duration timer sample
    */
    template<class... AN>
    static auto timer(rxsc::scheduler::clock_type::duration after, AN**...)
        -> decltype(rxs::timer(after)) {
        return      rxs::timer(after);
        static_assert(sizeof...(AN) == 0, "timer(after) was passed too many arguments.");
    }
    /*! Returns an observable that emits an integer at the specified time point, on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param  when  time point when the value is emitted
        \param  cn    the scheduler to use for scheduling the items

        \return  Observable that emits an integer at the specified time point

        \sample
        \snippet timer.cpp threaded timepoint timer sample
        \snippet output.txt threaded timepoint timer sample
    */
    template<class Coordination>
    static auto timer(rxsc::scheduler::clock_type::time_point when, Coordination cn)
        -> decltype(rxs::timer(when, std::move(cn))) {
        return      rxs::timer(when, std::move(cn));
    }
    /*! Returns an observable that emits an integer in the specified time interval, on the specified scheduler.

        \tparam Coordination  the type of the scheduler

        \param  when  interval when the value is emitted
        \param  cn    the scheduler to use for scheduling the items

        \return  Observable that emits an integer in the specified time interval

        \sample
        \snippet timer.cpp threaded duration timer sample
        \snippet output.txt threaded duration timer sample
    */
    template<class Coordination>
    static auto timer(rxsc::scheduler::clock_type::duration when, Coordination cn)
        -> decltype(rxs::timer(when, std::move(cn))) {
        return      rxs::timer(when, std::move(cn));
    }
    /*! Returns an observable that sends each value in the collection.

        \tparam Collection  the type of the collection of values that this observable emits

        \param  c  collection containing values to send

        \return  Observable that sends each value in the collection.

        \sample
        \snippet iterate.cpp iterate sample
        \snippet output.txt iterate sample
    */
    template<class Collection>
    static auto iterate(Collection c)
        -> decltype(rxs::iterate(std::move(c), identity_current_thread())) {
        return      rxs::iterate(std::move(c), identity_current_thread());
    }
    /*! Returns an observable that sends each value in the collection, on the specified scheduler.

        \tparam Collection    the type of the collection of values that this observable emits
        \tparam Coordination  the type of the scheduler

        \param  c   collection containing values to send
        \param  cn  the scheduler to use for scheduling the items

        \return  Observable that sends each value in the collection.

        \sample
        \snippet iterate.cpp threaded iterate sample
        \snippet output.txt threaded iterate sample
    */
    template<class Collection, class Coordination>
    static auto iterate(Collection c, Coordination cn)
        -> decltype(rxs::iterate(std::move(c), std::move(cn))) {
        return      rxs::iterate(std::move(c), std::move(cn));
    }
    /*! Returns an observable that sends an empty set of values and then completes.

        \tparam T  the type of elements (not) to be sent

        \return  Observable that sends an empty set of values and then completes.

        This is a degenerate case of rxcpp::observable<void,void>#from(Value0,ValueN...) operator.

        \note This is a degenerate case of ```observable<void,void>::from(Value0 v0, ValueN... vn)``` operator.
    */
    template<class T>
    static auto from()
        -> decltype(    rxs::from<T>()) {
        return          rxs::from<T>();
    }
    /*! Returns an observable that sends an empty set of values and then completes, on the specified scheduler.

        \tparam T  the type of elements (not) to be sent
        \tparam Coordination  the type of the scheduler

        \return  Observable that sends an empty set of values and then completes.

        \note This is a degenerate case of ```observable<void,void>::from(Coordination cn, Value0 v0, ValueN... vn)``` operator.
    */
    template<class T, class Coordination>
    static auto from(Coordination cn)
        -> typename std::enable_if<is_coordination<Coordination>::value,
            decltype(   rxs::from<T>(std::move(cn)))>::type {
        return          rxs::from<T>(std::move(cn));
    }
    /*! Returns an observable that sends each value from its arguments list.

        \tparam Value0  ...
        \tparam ValueN  the type of sending values

        \param  v0  ...
        \param  vn  values to send

        \return  Observable that sends each value from its arguments list.

        \sample
        \snippet from.cpp from sample
        \snippet output.txt from sample

        \note This operator is useful to send separated values. If they are stored as a collection, use observable<void,void>::iterate instead.
    */
    template<class Value0, class... ValueN>
    static auto from(Value0 v0, ValueN... vn)
        -> typename std::enable_if<!is_coordination<Value0>::value,
            decltype(   rxs::from(v0, vn...))>::type {
        return          rxs::from(v0, vn...);
    }
    /*! Returns an observable that sends each value from its arguments list, on the specified scheduler.

        \tparam Coordination  the type of the scheduler
        \tparam Value0  ...
        \tparam ValueN  the type of sending values

        \param  cn  the scheduler to use for scheduling the items
        \param  v0  ...
        \param  vn  values to send

        \return  Observable that sends each value from its arguments list.

        \sample
        \snippet from.cpp threaded from sample
        \snippet output.txt threaded from sample

        \note This operator is useful to send separated values. If they are stored as a collection, use observable<void,void>::iterate instead.
    */
    template<class Coordination, class Value0, class... ValueN>
    static auto from(Coordination cn, Value0 v0, ValueN... vn)
        -> typename std::enable_if<is_coordination<Coordination>::value,
            decltype(   rxs::from(std::move(cn), v0, vn...))>::type {
        return          rxs::from(std::move(cn), v0, vn...);
    }
    /*! Returns an observable that sends no items to observer and immediately completes.

        \tparam T             the type of (not) emitted items

        \return  Observable that sends no items to observer and immediately completes.

        \sample
        \snippet empty.cpp empty sample
        \snippet output.txt empty sample
    */
    template<class T>
    static auto empty()
        -> decltype(from<T>()) {
        return      from<T>();
    }
    /*! Returns an observable that sends no items to observer and immediately completes, on the specified scheduler.

        \tparam T             the type of (not) emitted items
        \tparam Coordination  the type of the scheduler

        \param  cn  the scheduler to use for scheduling the items

        \return  Observable that sends no items to observer and immediately completes.

        \sample
        \snippet empty.cpp threaded empty sample
        \snippet output.txt threaded empty sample
    */
    template<class T, class Coordination>
    static auto empty(Coordination cn)
        -> decltype(from<T>(std::move(cn))) {
        return      from<T>(std::move(cn));
    }
    /*! Returns an observable that sends the specified item to observer and then completes.

        \tparam T  the type of the emitted item

        \param v  the value to send

        \return  Observable that sends the specified item to observer and then completes.

        \sample
        \snippet just.cpp just sample
        \snippet output.txt just sample
    */
    template<class T>
    static auto just(T v)
        -> decltype(from(std::move(v))) {
        return      from(std::move(v));
    }
    /*! Returns an observable that sends the specified item to observer and then completes, on the specified scheduler.

        \tparam T             the type of the emitted item
        \tparam Coordination  the type of the scheduler

        \param v   the value to send
        \param cn  the scheduler to use for scheduling the items

        \return  Observable that sends the specified item to observer and then completes.

        \sample
        \snippet just.cpp threaded just sample
        \snippet output.txt threaded just sample
    */
    template<class T, class Coordination>
    static auto just(T v, Coordination cn)
        -> decltype(from(std::move(cn), std::move(v))) {
        return      from(std::move(cn), std::move(v));
    }
    /*! Returns an observable that sends no items to observer and immediately generates an error.

        \tparam T          the type of (not) emitted items
        \tparam Exception  the type of the error

        \param  e  the error to be passed to observers

        \return  Observable that sends no items to observer and immediately generates an error.

        \sample
        \snippet error.cpp error sample
        \snippet output.txt error sample
    */
    template<class T, class Exception>
    static auto error(Exception&& e)
        -> decltype(rxs::error<T>(std::forward<Exception>(e))) {
        return      rxs::error<T>(std::forward<Exception>(e));
    }
    /*! Returns an observable that sends no items to observer and immediately generates an error, on the specified scheduler.

        \tparam T             the type of (not) emitted items
        \tparam Exception     the type of the error
        \tparam Coordination  the type of the scheduler

        \param  e   the error to be passed to observers
        \param  cn  the scheduler to use for scheduling the items

        \return  Observable that sends no items to observer and immediately generates an error.

        \sample
        \snippet error.cpp threaded error sample
        \snippet output.txt threaded error sample
    */
    template<class T, class Exception, class Coordination>
    static auto error(Exception&& e, Coordination cn)
        -> decltype(rxs::error<T>(std::forward<Exception>(e), std::move(cn))) {
        return      rxs::error<T>(std::forward<Exception>(e), std::move(cn));
    }
    /*! Returns an observable that sends the specified values before it begins to send items emitted by the given observable.

        \tparam Observable  the type of the observable that emits values for resending
        \tparam Value0      ...
        \tparam ValueN      the type of sending values

        \param  o   the observable that emits values for resending
        \param  v0  ...
        \param  vn  values to send

        \return  Observable that sends the specified values before it begins to send items emitted by the given observable.

        \sample
        \snippet start_with.cpp full start_with sample
        \snippet output.txt full start_with sample

        Instead of passing the observable as a parameter, you can use rxcpp::observable<T, SourceOperator>::start_with method of the existing observable:
        \snippet start_with.cpp short start_with sample
        \snippet output.txt short start_with sample
    */
    template<class Observable, class Value0, class... ValueN>
    static auto start_with(Observable o, Value0 v0, ValueN... vn)
        -> decltype(rxs::from(rxu::value_type_t<Observable>(v0), rxu::value_type_t<Observable>(vn)...).concat(o)) {
        return      rxs::from(rxu::value_type_t<Observable>(v0), rxu::value_type_t<Observable>(vn)...).concat(o);
    }
    /*! Returns an observable that makes an observable by the specified observable factory
        using the resource provided by the specified resource factory for each new observer that subscribes.

        \tparam ResourceFactory    the type of the resource factory
        \tparam ObservableFactory  the type of the observable factory

        \param  rf  the resource factory function that resturn the rxcpp::resource that is used as a resource by the observable factory
        \param  of  the observable factory function to invoke for each observer that subscribes to the resulting observable

        \return  observable that makes an observable by the specified observable factory
                 using the resource provided by the specified resource factory for each new observer that subscribes.

        \sample
        \snippet scope.cpp scope sample
        \snippet output.txt scope sample
    */
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
