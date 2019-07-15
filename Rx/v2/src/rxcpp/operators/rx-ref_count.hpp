// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-ref_count.hpp

    \brief  Make some \c connectable_observable behave like an ordinary \c observable.
            Uses a reference count of the subscribers to control the connection to the published observable.

            The first subscription will cause a call to \c connect(), and the last \c unsubscribe will unsubscribe the connection.

            There are 2 variants of the operator:
            \li \c ref_count(): calls \c connect on the \c source \c connectable_observable.
            \li \c ref_count(other): calls \c connect on the \c other \c connectable_observable.

    \tparam ConnectableObservable the type of the \c other \c connectable_observable (optional)
    \param  other \c connectable_observable to call \c connect on (optional)

    If \c other is omitted, then \c source is used instead (which must be a \c connectable_observable).
    Otherwise, \c source can be a regular \c observable.

    \return An \c observable that emits the items from its \c source.

    \sample
    \snippet ref_count.cpp ref_count other diamond sample
    \snippet output.txt ref_count other diamond sample
 */

#if !defined(RXCPP_OPERATORS_RX_REF_COUNT_HPP)
#define RXCPP_OPERATORS_RX_REF_COUNT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct ref_count_invalid_arguments {};

template<class... AN>
struct ref_count_invalid : public rxo::operator_base<ref_count_invalid_arguments<AN...>> {
    using type = observable<ref_count_invalid_arguments<AN...>, ref_count_invalid<AN...>>;
};
template<class... AN>
using ref_count_invalid_t = typename ref_count_invalid<AN...>::type;

// ref_count(other) takes a regular observable source, not a connectable_observable.
// use template specialization to avoid instantiating 'subscribe' for two different types
// which would cause a compilation error.
template <typename connectable_type, typename observable_type>
struct ref_count_state_base {
    ref_count_state_base(connectable_type other, observable_type source)
        : connectable(std::move(other))
        , subscribable(std::move(source)) {}

    connectable_type connectable; // connects to this. subscribes to this if subscribable empty.
    observable_type subscribable; // subscribes to this if non-empty.

    template <typename Subscriber>
    void subscribe(Subscriber&& o) {
        subscribable.subscribe(std::forward<Subscriber>(o));
    }
};

// Note: explicit specializations have to be at namespace scope prior to C++17.
template <typename connectable_type>
struct ref_count_state_base<connectable_type, void> {
    explicit ref_count_state_base(connectable_type c)
        : connectable(std::move(c)) {}

    connectable_type connectable; // connects to this. subscribes to this if subscribable empty.

    template <typename Subscriber>
    void subscribe(Subscriber&& o) {
        connectable.subscribe(std::forward<Subscriber>(o));
    }
};

template<class T,
         class ConnectableObservable,
         class Observable = void> // note: type order flipped versus the operator.
struct ref_count : public operator_base<T>
{
    typedef rxu::decay_t<Observable> observable_type;
    typedef rxu::decay_t<ConnectableObservable> connectable_type;

    // ref_count() == false
    // ref_count(other) == true
    using has_observable_t = rxu::negation<std::is_same<void, Observable>>;
    // removed constexpr to support older VC compilers
    static /*constexpr */ const bool has_observable_v = has_observable_t::value;

    struct ref_count_state : public std::enable_shared_from_this<ref_count_state>,
                             public ref_count_state_base<ConnectableObservable, Observable>
    {
        template <class HasObservable = has_observable_t,
                  class Enabled = rxu::enable_if_all_true_type_t<
                      rxu::negation<HasObservable>>>
        explicit ref_count_state(connectable_type source)
            : ref_count_state_base<ConnectableObservable, Observable>(std::move(source))
            , subscribers(0)
        {
        }

        template <bool HasObservableV = has_observable_v>
        ref_count_state(connectable_type other,
                        typename std::enable_if<HasObservableV, observable_type>::type source)
            : ref_count_state_base<ConnectableObservable, Observable>(std::move(other),
                                                                      std::move(source))
            , subscribers(0)
        {
        }

        std::mutex lock;
        long subscribers;
        composite_subscription connection;
    };
    std::shared_ptr<ref_count_state> state;

    // connectable_observable<T> source = ...;
    // source.ref_count();
    //
    // calls connect on source after the subscribe on source.
    template <class HasObservable = has_observable_t,
              class Enabled = rxu::enable_if_all_true_type_t<
                  rxu::negation<HasObservable>>>
    explicit ref_count(connectable_type source)
        : state(std::make_shared<ref_count_state>(std::move(source)))
    {
    }

    // connectable_observable<?> other = ...;
    // observable<T> source = ...;
    // source.ref_count(other);
    //
    // calls connect on 'other' after the subscribe on 'source'.
    template <bool HasObservableV = has_observable_v>
    ref_count(connectable_type other,
              typename std::enable_if<HasObservableV, observable_type>::type source)
        : state(std::make_shared<ref_count_state>(std::move(other), std::move(source)))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber&& o) const {
        std::unique_lock<std::mutex> guard(state->lock);
        auto needConnect = ++state->subscribers == 1;
        auto keepAlive = state;
        guard.unlock();
        o.add(
            [keepAlive](){
                std::unique_lock<std::mutex> guard_unsubscribe(keepAlive->lock);
                if (--keepAlive->subscribers == 0) {
                    keepAlive->connection.unsubscribe();
                    keepAlive->connection = composite_subscription();
                }
            });
        keepAlive->subscribe(std::forward<Subscriber>(o));
        if (needConnect) {
            keepAlive->connectable.connect(keepAlive->connection);
        }
    }
};

}

/*! @copydoc rx-ref_count.hpp
*/
template<class... AN>
auto ref_count(AN&&... an)
    ->     operator_factory<ref_count_tag, AN...> {
    return operator_factory<ref_count_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}   
    
}

template<>
struct member_overload<ref_count_tag>
{
    template<class ConnectableObservable,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_connectable_observable<ConnectableObservable>>,
        class SourceValue = rxu::value_type_t<ConnectableObservable>,
        class RefCount = rxo::detail::ref_count<SourceValue, rxu::decay_t<ConnectableObservable>>,
        class Value = rxu::value_type_t<RefCount>,
        class Result = observable<Value, RefCount>
        >
    static Result member(ConnectableObservable&& o) {
        return Result(RefCount(std::forward<ConnectableObservable>(o)));
    }

    template<class Observable,
        class ConnectableObservable,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>,
            is_connectable_observable<ConnectableObservable>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class RefCount = rxo::detail::ref_count<SourceValue,
            rxu::decay_t<ConnectableObservable>,
            rxu::decay_t<Observable>>,
        class Value = rxu::value_type_t<RefCount>,
        class Result = observable<Value, RefCount>
        >
    static Result member(Observable&& o, ConnectableObservable&& other) {
        return Result(RefCount(std::forward<ConnectableObservable>(other),
                               std::forward<Observable>(o)));
    }

    template<class... AN>
    static operators::detail::ref_count_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "ref_count takes (optional ConnectableObservable)");
    }
};
    
}

#endif
