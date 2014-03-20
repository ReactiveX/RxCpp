// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SUBSCRIBER_HPP)
#define RXCPP_RX_SUBSCRIBER_HPP

#include "rx-includes.hpp"

namespace rxcpp {

template<class T>
struct subscriber_base : public observer_base<T>, public subscription_base, public resumption_base
{
    typedef tag_subscriber subscriber_tag;
};

template<class T, class Observer = observer<T>>
class subscriber : public subscriber_base<T>
{
    typedef subscriber<T, Observer> this_type;
    typedef typename std::decay<Observer>::type observer_type;

    composite_subscription lifetime;
    resumption controller;
    observer_type destination;

    struct detacher
    {
        ~detacher()
        {
            if (that) {
                that->unsubscribe();
            }
        }
        detacher(const this_type* that)
            : that(that)
        {
        }
        const this_type* that;
    };

public:
    typedef typename composite_subscription::weak_subscription weak_subscription;
    typedef typename composite_subscription::shared_subscription shared_subscription;

    subscriber()
    {
    }
    template<class U>
    subscriber(composite_subscription cs, resumption r, U&& o)
        : lifetime(std::move(cs))
        , controller(std::move(r))
        , destination(std::forward<U>(o))
    {
    }

    const observer_type& get_observer() const {
        return destination;
    }
    observer_type& get_observer() {
        return destination;
    }
    const resumption& get_resumption() const {
        return controller;
    }
    resumption& get_resumption() {
        return controller;
    }
    const composite_subscription& get_subscription() const {
        return lifetime;
    }
    composite_subscription& get_subscription() {
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
        if (is_subscribed()) {
            detacher protect(this);
            destination.on_next(std::move(t));
            protect.that = nullptr;
        }
    }
    void on_error(std::exception_ptr e) const {
        if (is_subscribed()) {
            detacher protect(this);
            destination.on_error(e);
        }
    }
    void on_completed() const {
        if (is_subscribed()) {
            detacher protect(this);
            destination.on_completed();
        }
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

namespace detail {

template<class T, bool subscriber_is_arg, bool observer_is_arg, bool onnext_is_arg>
struct observer_selector;

template<class T, bool subscriber_is_arg>
struct observer_selector<T, subscriber_is_arg, true, false>
{
    template<class Set>
    static auto get_observer(Set&& rs)
        -> decltype(std::get<5>(std::forward<Set>(rs)).value) {
        return      std::get<5>(std::forward<Set>(rs)).value;
    }
};
template<class T, bool subscriber_is_arg>
struct observer_selector<T, subscriber_is_arg, false, true>
{
    template<class Set>
    static auto get_observer(Set&& rs)
        -> decltype(make_observer_resolved<T>(std::forward<Set>(rs))) {
        return      make_observer_resolved<T>(std::forward<Set>(rs));
    }
};
template<class T>
struct observer_selector<T, true, false, false>
{
    template<class Set>
    static auto get_observer(Set&& rs)
        -> const decltype(  std::get<6>(std::forward<Set>(rs)).value.get_observer())& {
        return              std::get<6>(std::forward<Set>(rs)).value.get_observer();
    }
};

template<class T, class ResolvedArgSet>
auto select_observer(ResolvedArgSet&& rs)
    -> decltype(observer_selector<T, std::decay<decltype(std::get<6>(std::forward<ResolvedArgSet>(rs)))>::type::is_arg, std::decay<decltype(std::get<5>(std::forward<ResolvedArgSet>(rs)))>::type::is_arg, std::decay<decltype(std::get<0>(std::forward<ResolvedArgSet>(rs)))>::type::is_arg>::get_observer(std::forward<ResolvedArgSet>(rs))) {
    return      observer_selector<T, std::decay<decltype(std::get<6>(std::forward<ResolvedArgSet>(rs)))>::type::is_arg, std::decay<decltype(std::get<5>(std::forward<ResolvedArgSet>(rs)))>::type::is_arg, std::decay<decltype(std::get<0>(std::forward<ResolvedArgSet>(rs)))>::type::is_arg>::get_observer(std::forward<ResolvedArgSet>(rs));

    typedef typename std::decay<decltype(std::get<4>(std::forward<ResolvedArgSet>(rs)))>::type rr_t;
    typedef typename std::decay<decltype(std::get<0>(std::forward<ResolvedArgSet>(rs)))>::type rn_t;
    typedef typename std::decay<decltype(std::get<1>(std::forward<ResolvedArgSet>(rs)))>::type re_t;
    typedef typename std::decay<decltype(std::get<2>(std::forward<ResolvedArgSet>(rs)))>::type rc_t;
    typedef typename std::decay<decltype(std::get<5>(std::forward<ResolvedArgSet>(rs)))>::type ro_t;
    typedef typename std::decay<decltype(std::get<6>(std::forward<ResolvedArgSet>(rs)))>::type rs_t;

    static_assert(rs_t::is_arg || ro_t::is_arg || rn_t::is_arg, "at least one of; onnext, observer or subscriber is required");
    static_assert(int(ro_t::is_arg) + int(rn_t::is_arg) < 2, "onnext, onerror and oncompleted not allowed with an observer");
}

template<class T, class ResolvedArgSet>
auto make_subscriber_resolved(ResolvedArgSet&& rsArg)
    ->      subscriber<T, decltype(     select_observer<T>(std::move(rsArg)))> {
    const auto rs = std::forward<ResolvedArgSet>(rsArg);
    const auto rsub = std::get<3>(rs);
    const auto rr = std::get<4>(rs);
    const auto rscrbr = std::get<6>(rs);
    const auto r = (rscrbr.is_arg && !rr.is_arg)      ? rscrbr.value.get_resumption()       : rr.value;
    const auto s = (rscrbr.is_arg && !rsub.is_arg)    ? rscrbr.value.get_subscription()     : rsub.value;
    return  subscriber<T, decltype(     select_observer<T>(std::move(rsArg)))>(
            s, r, select_observer<T>(std::move(rs)));

// at least for now do not enforce resolver
#if 0
    typedef typename std::decay<decltype(rr)>::type rr_t;
    typedef typename std::decay<decltype(rscrbr)>::type rs_t;

    static_assert(rs_t::is_arg || rr_t::is_arg, "at least one of; resumption or subscriber is a required parameter");
#endif
}

struct tag_subscription_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = !is_subscriber<LHS>::value && !is_observer<LHS>::value && is_subscription<LHS>::value;
    };
    typedef composite_subscription default_type;
};

template<class T>
struct tag_subscriber_resolution
{
    template<class LHS>
    struct predicate : public is_subscriber<LHS>
    {
    };
    typedef subscriber<T, observer<T, void>> default_type;
};

template<class T>
struct tag_observer_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = !is_subscriber<LHS>::value && is_observer<LHS>::value;
    };
    typedef observer<T, void> default_type;
};

struct tag_resumption_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = !is_subscriber<LHS>::value && is_resumption<LHS>::value;
    };
    typedef resumption default_type;
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
                // the first four must be the same as tag_observer_set or the indexing will fail
    : public    rxu::detail::tag_set<tag_onnext_resolution<T>,
                rxu::detail::tag_set<tag_onerror_resolution,
                rxu::detail::tag_set<tag_oncompleted_resolution,
                rxu::detail::tag_set<tag_subscription_resolution,
                rxu::detail::tag_set<tag_resumption_resolution,
                rxu::detail::tag_set<tag_observer_resolution<T>,
                rxu::detail::tag_set<tag_subscriber_resolution<T>>>>>>>>
{
};

}

#if RXCPP_USE_VARIADIC_TEMPLATES
template<class T, class Arg0, class... ArgN>
auto make_subscriber(Arg0&& a0, ArgN&&... an)
    -> decltype(detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...))) {
    return      detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
}
#else
template<class T, class Arg0>
auto make_subscriber(Arg0&& a0)
    -> decltype(detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0)))) {
    return      detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0)));
}
template<class T, class Arg0, class Arg1>
auto make_subscriber(Arg0&& a0, Arg1&& a1)
    -> decltype(detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1)))) {
    return      detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1)));
}
template<class T, class Arg0, class Arg1, class Arg2>
auto make_subscriber(Arg0&& a0, Arg1&& a1, Arg2&& a2)
    -> decltype(detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)))) {
    return      detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3>
auto make_subscriber(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3)
    -> decltype(detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))) {
    return      detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
auto make_subscriber(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4)
    -> decltype(detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)))) {
    return      detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4)));
}
template<class T, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
auto make_subscriber(Arg0&& a0, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5)
    -> decltype(detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)))) {
    return      detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(detail::tag_subscriber_set<T>(), std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3), std::forward<Arg4>(a4), std::forward<Arg5>(a5)));
}
#endif

}

#endif
