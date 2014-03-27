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
    template<class V>
    void on_next(V&& v) const {
        if (!is_subscribed()) {
            abort();
        }
        detacher protect(this);
        destination.on_next(std::forward<V>(v));
        protect.that = nullptr;
    }
    void on_error(std::exception_ptr e) const {
        if (!is_subscribed()) {
            abort();
        }
        detacher protect(this);
        destination.on_error(e);
    }
    void on_completed() const {
        if (!is_subscribed()) {
            abort();
        }
        detacher protect(this);
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

namespace detail {

template<class T, bool subscriber_is_arg, bool observer_is_arg, bool onnext_is_arg>
struct observer_selector;

template<class T, bool subscriber_is_arg>
struct observer_selector<T, subscriber_is_arg, true, false>
{
    template<class Set>
    static auto get_observer(Set& rs)
        -> decltype(std::get<5>(rs).value) {
        return      std::get<5>(rs).value;
    }
};
template<class T, bool subscriber_is_arg>
struct observer_selector<T, subscriber_is_arg, false, true>
{
    template<class Set>
    static auto get_observer(Set& rs)
        -> decltype(make_observer_resolved<T>(rs)) {
        return      make_observer_resolved<T>(rs);
    }
};
template<class T>
struct observer_selector<T, true, false, false>
{
    template<class Set>
    static auto get_observer(Set& rs)
        -> decltype(    std::get<6>(rs).value.get_observer()) {
        return          std::get<6>(rs).value.get_observer();
    }
};

template<class T, class ResolvedArgSet>
auto select_observer(ResolvedArgSet& rs)
    -> decltype(observer_selector<T, std::decay<decltype(std::get<6>(rs))>::type::is_arg, std::decay<decltype(std::get<5>(rs))>::type::is_arg, std::decay<decltype(std::get<0>(rs))>::type::is_arg>::get_observer(rs)) {
    return      observer_selector<T, std::decay<decltype(std::get<6>(rs))>::type::is_arg, std::decay<decltype(std::get<5>(rs))>::type::is_arg, std::decay<decltype(std::get<0>(rs))>::type::is_arg>::get_observer(rs);

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
    ->      subscriber<T, decltype(     select_observer<T>(*(typename std::decay<ResolvedArgSet>::type*)nullptr))> {
    const auto rs = std::forward<ResolvedArgSet>(rsArg);
    const auto rsub = std::get<3>(rs);
    const auto rr = std::get<4>(rs);
    const auto rscrbr = std::get<6>(rs);
    const auto r = (rscrbr.is_arg && !rr.is_arg)      ? rscrbr.value.get_resumption()       : rr.value;
    const auto s = (rscrbr.is_arg && !rsub.is_arg)    ? rscrbr.value.get_subscription()     : rsub.value;
    return  subscriber<T, decltype(     select_observer<T>(*(typename std::decay<ResolvedArgSet>::type*)nullptr))>(
            s, r, select_observer<T>(rs));

    static_assert(std::tuple_size<decltype(rs)>::value == 7, "must have 7 resolved args");
// at least for now do not enforce resolver
#if 0
    typedef typename std::decay<decltype(rr)>::type rr_t;
    typedef typename std::decay<decltype(rscrbr)>::type rs_t;

    static_assert(rs_t::is_arg || rr_t::is_arg, "at least one of; resumption or subscriber is a required parameter");
#endif
}

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
{
    // the first four must be the same as tag_observer_set or the indexing will fail
    typedef rxu::detail::tag_set<
        tag_onnext_resolution<T>,
        tag_onerror_resolution,
        tag_oncompleted_resolution,
        tag_subscription_resolution,
        tag_resumption_resolution,
        tag_observer_resolution<T>,
        tag_subscriber_resolution<T>> type;
};

}

template<class T, class Arg0, class... ArgN>
auto make_subscriber(Arg0&& a0, ArgN&&... an)
    -> decltype(detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(typename detail::tag_subscriber_set<T>::type(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...))) {
    return      detail::make_subscriber_resolved<T>(rxu::detail::resolve_arg_set(typename detail::tag_subscriber_set<T>::type(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
}

}

#endif
