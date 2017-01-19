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

    /*! @copydoc rxcpp::operators::is_empty
     */
    template<class... AN>
    auto is_empty(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(is_empty_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(is_empty_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-any.hpp
     */
    template<class... AN>
    auto any(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(any_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(any_tag{},                *this, std::forward<AN>(an)...);
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

    /*! @copydoc rx-switch_if_empty.hpp
    */
    template<class... AN>
    auto switch_if_empty(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(switch_if_empty_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(switch_if_empty_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rxcpp::operators::default_if_empty
    */
    template<class... AN>
    auto default_if_empty(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(default_if_empty_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(default_if_empty_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-sequence_equal.hpp
     */
    template<class... AN>
    auto sequence_equal(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(sequence_equal_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(sequence_equal_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-tap.hpp
     */
    template<class... AN>
    auto tap(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(tap_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(tap_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-time_interval.hpp
     */
    template<class... AN>
    auto time_interval(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(time_interval_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(time_interval_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-timeout.hpp
     */
    template<class... AN>
    auto timeout(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(timeout_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(timeout_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-timestamp.hpp
     */
    template<class... AN>
    auto timestamp(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(timestamp_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(timestamp_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-finally.hpp
     */
    template<class... AN>
    auto finally(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(finally_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(finally_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-on_error_resume_next.hpp
    */
    template<class... AN>
    auto on_error_resume_next(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(on_error_resume_next_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(on_error_resume_next_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-map.hpp
     */
    template<class... AN>
    auto map(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(map_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(map_tag{},                *this, std::forward<AN>(an)...);
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

    /*! @copydoc rx-window.hpp
    */
    template<class... AN>
    auto window(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(window_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(window_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-window_time.hpp
    */
    template<class... AN>
    auto window_with_time(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(window_with_time_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(window_with_time_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-window_time_count.hpp
    */
    template<class... AN>
    auto window_with_time_or_count(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(window_with_time_or_count_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(window_with_time_or_count_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-window_toggle.hpp
    */
    template<class... AN>
    auto window_toggle(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(window_toggle_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(window_toggle_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-buffer_count.hpp
     */
    template<class... AN>
    auto buffer(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(buffer_count_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(buffer_count_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-buffer_time.hpp
     */
    template<class... AN>
    auto buffer_with_time(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(buffer_with_time_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(buffer_with_time_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-buffer_time_count.hpp
     */
    template<class... AN>
    auto buffer_with_time_or_count(AN&&... an) const
    /// \cond SHOW_SERVICE_MEMBERS
    -> decltype(observable_member(buffer_with_time_or_count_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
    /// \endcond
    {
        return  observable_member(buffer_with_time_or_count_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-switch_on_next.hpp
    */
    template<class... AN>
    auto switch_on_next(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(switch_on_next_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(switch_on_next_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-merge.hpp
     */
    template<class... AN>
    auto merge(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(merge_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(merge_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-amb.hpp
     */
    template<class... AN>
    auto amb(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(amb_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(amb_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-flat_map.hpp
     */
    template<class... AN>
    auto flat_map(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(flat_map_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(flat_map_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-concat.hpp
     */
    template<class... AN>
    auto concat(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(concat_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(concat_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-concat_map.hpp
     */
    template<class... AN>
    auto concat_map(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(concat_map_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(concat_map_tag{},                *this, std::forward<AN>(an)...);
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

    /*! @copydoc rx-scan.hpp
    */
    template<class... AN>
    auto scan(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(scan_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(scan_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-sample_time.hpp
     */
    template<class... AN>
    auto sample_with_time(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(sample_with_time_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(sample_with_time_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-skip.hpp
    */
    template<class... AN>
    auto skip(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(skip_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(skip_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-skip_last.hpp
    */
    template<class... AN>
    auto skip_last(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(skip_last_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(skip_last_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-skip_until.hpp
    */
    template<class... AN>
    auto skip_until(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(skip_until_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(skip_until_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-take.hpp
     */
    template<class... AN>
    auto take(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(take_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(take_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-take_last.hpp
    */
    template<class... AN>
    auto take_last(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(take_last_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(take_last_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-take_until.hpp
    */
    template<class... AN>
    auto take_until(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(take_until_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(take_until_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-take_while.hpp
    */
    template<class... AN>
    auto take_while(AN&&... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(take_while_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(take_while_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-repeat.hpp
     */
    template<class... AN>
    auto repeat(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(repeat_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(repeat_tag{},                *this, std::forward<AN>(an)...);
    }

    /*! @copydoc rx-retry.hpp
     */
    template<class... AN>
    auto retry(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(retry_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(retry_tag{},                *this, std::forward<AN>(an)...);
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

    /*! @copydoc rx-pairwise.hpp
     */
    template<class... AN>
    auto pairwise(AN... an) const
        /// \cond SHOW_SERVICE_MEMBERS
        -> decltype(observable_member(pairwise_tag{}, *(this_type*)nullptr, std::forward<AN>(an)...))
        /// \endcond
    {
        return      observable_member(pairwise_tag{},                *this, std::forward<AN>(an)...);
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
