// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SUBSCRIBER_HPP)
#define RXCPP_RX_SUBSCRIBER_HPP

#include "rx-includes.hpp"

namespace rxcpp {

template<class T, class Observer = observer<T>>
class subscriber : public observer_root<T>, public subscription_base
{
    composite_subscription lifetime;
    resumption controller;
    Observer destination;

public:
    typedef typename composite_subscription::weak_subscription weak_subscription;
    typedef typename composite_subscription::shared_subscription shared_subscription;

    subscriber()
    {
    }
    subscriber(composite_subscription cs, resumption r, Observer o)
        : lifetime(std::move(cs))
        , destination(std::move(o))
        , controller(std::move(r))
    {
    }

    Observer get_observer() const {
        return destination;
    }
    resumption get_resumption() const {
        return controller;
    }
    composite_subscription get_subscription() const {
        return lifetime;
    }

    // resumption
    //
    bool is_resumed() const {
        return controller.is_resumed();
    }
    void resume_with(rxsc::schedulable rw) {
        controller.resume_with(std::move(rw));
    }

    // observer
    //
    void on_next(T t) const {
        destination.on_next(std::move(t));
    }
    void on_error(std::exception_ptr e) const {
        destination.on_error(e);
    }
    void on_completed() const {
        destination.on_completed();
    }

    // composite_subscription
    //
    bool is_subscribed() const {
        return lifetime.is_subscribed();
    }
    weak_subscription add(shared_subscription s) const {
        return lifetime.add(std::move(s));
    }
    weak_subscription add(dynamic_subscription s) const {
        return lifetime.add(std::move(s));
    }
    void remove(weak_subscription w) const {
        return lifetime.remove(std::move(w));
    }
    void clear() const {
        return lifetime.clear();
    }
    void unsubscribe() const {
        return lifetime.unsubscribe();
    }

};

template<class T, class ResolvedArgSet>
auto make_observer_resolved(ResolvedArgSet&& rs)
    ->      observer<T, static_observer<T,  decltype (std::get<2>(std::forward<ResolvedArgSet>(rs)).value), decltype (std::get<3>(std::forward<ResolvedArgSet>(rs)).value), decltype (std::get<4>(std::forward<ResolvedArgSet>(rs)).value)>> {
        typedef         static_observer<T,  decltype (std::get<2>(std::forward<ResolvedArgSet>(rs)).value), decltype (std::get<3>(std::forward<ResolvedArgSet>(rs)).value), decltype (std::get<4>(std::forward<ResolvedArgSet>(rs)).value)> inner_type;
    return  observer<T, inner_type>(inner_type(
                                            std::move(std::get<2>(std::forward<ResolvedArgSet>(rs)).value), std::move(std::get<3>(std::forward<ResolvedArgSet>(rs)).value), std::move(std::get<4>(std::forward<ResolvedArgSet>(rs)).value)));

    typedef typename std::decay<decltype(std::get<2>(std::forward<ResolvedArgSet>(rs)))>::type rn_t;
    typedef typename std::decay<decltype(std::get<3>(std::forward<ResolvedArgSet>(rs)))>::type re_t;
    typedef typename std::decay<decltype(std::get<4>(std::forward<ResolvedArgSet>(rs)))>::type rc_t;

    static_assert(rn_t::is_arg, "onnext is a required parameter");
    static_assert(!(rn_t::is_arg && re_t::is_arg) || rn_t::n + 1 == re_t::n, "onnext, onerror parameters must be together and in order");
    static_assert(!(re_t::is_arg && rc_t::is_arg) || re_t::n + 1 == rc_t::n, "onerror, oncompleted parameters must be together and in order");
    static_assert(!(rn_t::is_arg && rc_t::is_arg  && !re_t::is_arg) || rn_t::n + 1 == rc_t::n, "onnext, oncompleted parameters must be together and in order");
}

template<class T, class ResolvedArgSet>
auto make_subscriber_resolved(ResolvedArgSet&& rs)
    ->      subscriber<T, decltype(make_observer_resolved<T>(std::forward<ResolvedArgSet>(rs)))> {
    return  subscriber<T, decltype(make_observer_resolved<T>(std::forward<ResolvedArgSet>(rs)))>(std::move(std::get<0>(std::forward<ResolvedArgSet>(rs)).value), std::move(std::get<1>(std::forward<ResolvedArgSet>(rs)).value), make_observer_resolved<T>(std::forward<ResolvedArgSet>(rs)));

    typedef typename std::decay<decltype(std::get<1>(std::forward<ResolvedArgSet>(rs)))>::type rr_t;

    static_assert(rr_t::is_arg, "resumption is a required parameter");
}

struct tag_subscription_resolution
{
    template<class LHS>
    struct predicate : public is_subscription<LHS>
    {
    };
    typedef composite_subscription default_type;
};

struct tag_resumption_resolution
{
    template<class LHS>
    struct predicate : public is_resumption<LHS>
    {
    };
    typedef resumption default_type;
};


template<class T>
struct tag_onnext_resolution
{
    template<class LHS>
    struct predicate : public detail::is_on_next_of<T, LHS>
    {
    };
    typedef detail::OnNextEmpty<T> default_type;
};

struct tag_onerror_resolution
{
    template<class LHS>
    struct predicate : public detail::is_on_error<LHS>
    {
    };
    typedef detail::OnErrorEmpty default_type;
};

struct tag_oncompleted_resolution
{
    template<class LHS>
    struct predicate : public detail::is_on_completed<LHS>
    {
    };
    typedef detail::OnCompletedEmpty default_type;
};

// types to disambiguate
// subscriber with override for subscription, observer, resumption |
// optional subscriber +
// observer | on_next and optional on_error, on_completed +
// resumption +
// subscription | unsubscribe
//

template<class T>
struct tag_subscriber_set
    : public    rxu::detail::tag_set<tag_subscription_resolution,
                rxu::detail::tag_set<tag_resumption_resolution,
                rxu::detail::tag_set<tag_onnext_resolution<T>,
                rxu::detail::tag_set<tag_onerror_resolution,
                rxu::detail::tag_set<tag_oncompleted_resolution>>>>>
{
};

template<class T, class Arg0>
auto make_subscriber(Arg0&& a0)
    -> decltype(make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0)))) {
    return      make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0)));
}
template<class T, class Arg0, class Arg1>
auto make_subscriber(Arg0&& a0, Arg1&& a1)
    -> decltype(make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1)))) {
    return      make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1)));
}
template<class T, class Arg0, class Arg1, class Arg2>
auto make_subscriber(Arg0&& a0, Arg1&& a1, Arg2&& a2)
    -> decltype(make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))) {
    return      make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3>
auto make_subscriber(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3)
    -> decltype(make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))) {
    return      make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
auto make_subscriber(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4)
    -> decltype(make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)))) {
    return      make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
auto make_subscriber(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5)
    -> decltype(make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)))) {
    return      make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)));
}

}

#endif
