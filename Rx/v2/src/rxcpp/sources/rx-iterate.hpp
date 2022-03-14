// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_ITERATE_HPP)
#define RXCPP_SOURCES_RX_ITERATE_HPP

#include "../rx-includes.hpp"

/*! \file rx-iterate.hpp

    \brief Returns an observable that sends each value in the collection, on the specified scheduler.

    \tparam Collection    the type of the collection of values that this observable emits
    \tparam Coordination  the type of the scheduler (optional)

    \param  c   collection containing values to send
    \param  cn  the scheduler to use for scheduling the items (optional)

    \return  Observable that sends each value in the collection.

    \sample
    \snippet iterate.cpp iterate sample
    \snippet output.txt iterate sample

    \sample
    \snippet iterate.cpp threaded iterate sample
    \snippet output.txt threaded iterate sample

*/

namespace rxcpp {

namespace sources {

namespace detail {

template<class Collection>
struct is_iterable
{
    using collection_type = rxu::decay_t<Collection>;

    struct not_void {};
    template<class CC>
    static auto check(int) -> decltype(std::begin(std::declval<CC>()));
    template<class CC>
    static not_void check(...);

    static const bool value = !rxcpp::is_same_v<decltype(check<collection_type>(0)), not_void>;
};

template<class Collection>
struct iterate_traits
{
    // add const due to we don't want to modify original values!
    using collection_type = std::add_const_t<rxu::decay_t<Collection>>;
    using iterator_type   = rxu::decay_t<decltype(std::begin(std::declval<collection_type>()))>;
    using value_type      = rxu::value_type_t<std::iterator_traits<iterator_type>>;
};

template<class Collection, class Coordination>
struct iterate : public source_base<rxu::value_type_t<iterate_traits<Collection>>>
{
    using this_type = iterate<Collection, Coordination>;
    using traits = iterate_traits<Collection>;

    using coordination_type = rxu::decay_t<Coordination>;
    using coordinator_type = typename coordination_type::coordinator_type;

    using collection_type     = typename traits::collection_type;
    using collection_type_ptr = std::shared_ptr<collection_type>;
    using iterator_type       = typename traits::iterator_type;

    struct iterate_initial_type
    {
        iterate_initial_type(Collection&& c, coordination_type cn)
            : collection_ptr(std::make_shared<collection_type>(std::move(c)))
            , coordination(std::move(cn)) { }

        iterate_initial_type(const Collection& c, coordination_type cn)
            : collection_ptr(std::make_shared<collection_type>(c))
            , coordination(std::move(cn)) { }

        collection_type_ptr    collection_ptr;
        coordination_type      coordination;
    };
    iterate_initial_type initial;

    iterate(Collection&& c, coordination_type cn)
        : initial(std::move(c), std::move(cn)) { }

    iterate(const Collection& c, coordination_type cn)
        : initial(c, std::move(cn)) { }

    template<class Subscriber>
    void on_subscribe(Subscriber o) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        using output_type = typename coordinator_type::template get<Subscriber>::type;

        struct iterate_state_type
            : public iterate_initial_type
        {
            iterate_state_type(const iterate_initial_type& i, output_type o)
                : iterate_initial_type(i)
                , cursor(std::begin(*iterate_initial_type::collection_ptr))
                , end(std::end(*iterate_initial_type::collection_ptr))
                , out(std::move(o))
            {
            }
            iterate_state_type(const iterate_state_type& o)
                : iterate_initial_type(o)
                , cursor(std::begin(*iterate_initial_type::collection_ptr))
                , end(std::end(*iterate_initial_type::collection_ptr))
                , out(std::move(o.out)) // since lambda capture does not yet support move
            {
            }

            mutable iterator_type cursor;
            iterator_type end;
            mutable output_type out;
        };

        // creates a worker whose lifetime is the same as this subscription
        auto coordinator = initial.coordination.create_coordinator(o.get_subscription());

        iterate_state_type state(initial, o);

        auto controller = coordinator.get_worker();

        auto producer = [state](const rxsc::schedulable& self){
            if (!state.out.is_subscribed()) {
                // terminate loop
                return;
            }

            if (state.cursor != state.end) {
                // send next value
                state.out.on_next(*state.cursor);
                ++state.cursor;
            }

            if (state.cursor == state.end) {
                state.out.on_completed();
                // o is unsubscribed
                return;
            }

            // tail recurse this same action to continue loop
            self();
        };
        auto selectedProducer = on_exception(
            [&](){return coordinator.act(producer);},
            o);
        if (selectedProducer.empty()) {
            return;
        }
        controller.schedule(selectedProducer.get());

    }
};

}

/*! @copydoc rx-iterate.hpp
 */
template<class Collection>
auto iterate(Collection&& c)
    ->      observable<rxu::value_type_t<detail::iterate_traits<rxu::decay_t<Collection>>>, detail::iterate<rxu::decay_t<Collection>, identity_one_worker>> {
    return  observable<rxu::value_type_t<detail::iterate_traits<rxu::decay_t<Collection>>>, detail::iterate<rxu::decay_t<Collection>, identity_one_worker>>(
                                                                              detail::iterate<rxu::decay_t<Collection>, identity_one_worker>(std::forward<Collection>(c), identity_immediate()));
}
/*! @copydoc rx-iterate.hpp
 */
template<class Collection, class Coordination>
auto iterate(Collection&& c, Coordination cn)
    ->      observable<rxu::value_type_t<detail::iterate_traits<rxu::decay_t<Collection>>>, detail::iterate<rxu::decay_t<Collection>, Coordination>> {
    return  observable<rxu::value_type_t<detail::iterate_traits<rxu::decay_t<Collection>>>, detail::iterate<rxu::decay_t<Collection>, Coordination>>(
                                                                              detail::iterate<rxu::decay_t<Collection>, Coordination>(std::forward<Collection>(c), std::move(cn)));
}

/*! Returns an observable that sends an empty set of values and then completes.

    \tparam T  the type of elements (not) to be sent

    \return  Observable that sends an empty set of values and then completes.

    This is a degenerate case of rxcpp::observable<void,void>#from(Value0,ValueN...) operator.

    \note This is a degenerate case of ```from(Value0 v0, ValueN... vn)``` operator.
*/
template<class T>
auto from()
    -> decltype(iterate(std::initializer_list<T>(), identity_immediate())) {
    return      iterate(std::initializer_list<T>(), identity_immediate());
}
/*! Returns an observable that sends an empty set of values and then completes, on the specified scheduler.

    \tparam T  the type of elements (not) to be sent
    \tparam Coordination  the type of the scheduler

    \return  Observable that sends an empty set of values and then completes.

    \note This is a degenerate case of ```from(Coordination cn, Value0 v0, ValueN... vn)``` operator.
*/
template<class T, class Coordination>
auto from(Coordination cn)
    -> typename std::enable_if<is_coordination<Coordination>::value,
        decltype(   iterate(std::initializer_list<T>(), std::move(cn)))>::type {
    return          iterate(std::initializer_list<T>(), std::move(cn));
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
auto from(Value0&& v0, ValueN&&... vn)
    -> typename std::enable_if<!is_coordination<rxu::decay_t<Value0>>::value,
        decltype(iterate(std::declval<std::array<rxu::decay_t<Value0>, sizeof...(ValueN) + 1>>(), identity_immediate()))>::type {
    std::array<rxu::decay_t<Value0>, sizeof...(ValueN) + 1> c{{std::forward<Value0>(v0), std::forward<ValueN>(vn)...}};
    return iterate(std::move(c), identity_immediate());
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
auto from(Coordination cn, Value0&& v0, ValueN&&... vn)
    -> typename std::enable_if<is_coordination<Coordination>::value,
        decltype(iterate(std::declval<std::array<rxu::decay_t<Value0>, sizeof...(ValueN) + 1>>(), std::move(cn)))>::type {
    std::array<rxu::decay_t<Value0>, sizeof...(ValueN) + 1> c{{std::forward<Value0>(v0), std::forward<ValueN>(vn)...}};
    return iterate(std::move(c), std::move(cn));
}


/*! Returns an observable that sends the specified item to observer and then completes.

    \tparam T  the type of the emitted item

    \param v  the value to send

    \return  Observable that sends the specified item to observer and then completes.

    \sample
    \snippet just.cpp just sample
    \snippet output.txt just sample
*/
template<class Value0>
auto just(Value0&& v0)
    -> typename std::enable_if<!is_coordination<rxu::decay_t<Value0>>::value,
        decltype(iterate(std::declval<std::array<rxu::decay_t<Value0>, 1>>(), identity_immediate()))>::type {
    std::array<rxu::decay_t<Value0>, 1> c{{std::forward<Value0>(v0)}};
    return iterate(std::move(c), identity_immediate());
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
template<class Value0, class Coordination>
auto just(Value0&& v0, Coordination cn)
    -> typename std::enable_if<is_coordination<Coordination>::value,
        decltype(iterate(std::declval<std::array<rxu::decay_t<Value0>, 1>>(), std::move(cn)))>::type {
    std::array<rxu::decay_t<Value0>, 1> c{{std::forward<Value0>(v0)}};
    return iterate(std::move(c), std::move(cn));
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
auto start_with(Observable o, Value0&& v0, ValueN&&... vn)
    -> decltype(from(rxu::value_type_t<Observable>(std::forward<Value0>(v0)), rxu::value_type_t<Observable>(std::forward<ValueN>(vn))...).concat(o)) {
    return      from(rxu::value_type_t<Observable>(std::forward<Value0>(v0)), rxu::value_type_t<Observable>(std::forward<ValueN>(vn))...).concat(o);
}

}

}

#endif
