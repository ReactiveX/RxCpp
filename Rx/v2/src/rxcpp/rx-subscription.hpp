// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SUBSCRIPTION_HPP)
#define RXCPP_RX_SUBSCRIPTION_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace detail {

template<class F>
struct is_unsubscribe_function
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype(std::declval<CF>()());
    template<class CF>
    static not_void check(...);

    static const bool value = rxcpp::is_same_v<decltype(check<rxu::decay_t<F>>(0)), void>;
};

}

struct tag_subscription {};
struct subscription_base { using subscription_tag = tag_subscription; };
template<class T>
class is_subscription
{
    template<class C>
    static typename C::subscription_tag* check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_subscription*>::value;
};

template<class Unsubscribe>
class static_subscription
{
    using unsubscribe_call_type = rxu::decay_t<Unsubscribe>;
    unsubscribe_call_type unsubscribe_call;
    static_subscription()
    {
    }
public:
    static_subscription(const static_subscription& o)
        : unsubscribe_call(o.unsubscribe_call)
    {
    }
    static_subscription(static_subscription&& o)
        : unsubscribe_call(std::move(o.unsubscribe_call))
    {
    }
    static_subscription(unsubscribe_call_type s)
        : unsubscribe_call(std::move(s))
    {
    }
    void unsubscribe() const {
        unsubscribe_call();
    }
};

class subscription : public subscription_base
{
    class base_subscription_state : public std::enable_shared_from_this<base_subscription_state>
    {
        base_subscription_state();
    public:

        explicit base_subscription_state(bool initial)
            : issubscribed(initial)
        {
        }
        virtual ~base_subscription_state() {}
        virtual void unsubscribe() {
        }
        std::atomic<bool> issubscribed;
    };
public:
    using weak_state_type = std::weak_ptr<base_subscription_state>;

private:
    template<class I>
    struct subscription_state : public base_subscription_state
    {
        using inner_t = rxu::decay_t<I>;
        subscription_state(inner_t i)
            : base_subscription_state(true)
            , inner(std::move(i))
        {
        }
        virtual void unsubscribe() {
            if (issubscribed.exchange(false)) {
                trace_activity().unsubscribe_enter(*this);
                inner.unsubscribe();
                trace_activity().unsubscribe_return(*this);
            }
        }
        inner_t inner;
    };

protected:
    std::shared_ptr<base_subscription_state> state;

    friend bool operator<(const subscription&, const subscription&);
    friend bool operator==(const subscription&, const subscription&);

private:
    subscription(weak_state_type w)
        : state(w.lock())
    {
        if (!state) {
            std::terminate();
        }
    }

    explicit subscription(std::shared_ptr<base_subscription_state> s)
        : state(std::move(s))
    {
        if (!state) {
            std::terminate();
        }
    }
public:

    subscription()
        : state(std::make_shared<base_subscription_state>(false))
    {
        if (!state) {
            std::terminate();
        }
    }
    template<class U>
    explicit subscription(U u, typename std::enable_if<!is_subscription<U>::value, void**>::type = nullptr)
        : state(std::make_shared<subscription_state<U>>(std::move(u)))
    {
        if (!state) {
            std::terminate();
        }
    }
    template<class U>
    explicit subscription(U u, typename std::enable_if<!rxcpp::is_same_v<subscription, U> && is_subscription<U>::value, void**>::type = nullptr)
        // intentionally slice
        : state(std::move((*static_cast<subscription*>(&u)).state))
    {
        if (!state) {
            std::terminate();
        }
    }
    subscription(const subscription& o)
        : state(o.state)
    {
        if (!state) {
            std::terminate();
        }
    }
    subscription(subscription&& o)
        : state(std::move(o.state))
    {
        if (!state) {
            std::terminate();
        }
    }
    subscription& operator=(subscription o) {
        state = std::move(o.state);
        return *this;
    }
    bool is_subscribed() const {
        if (!state) {
            std::terminate();
        }
        return state->issubscribed;
    }
    void unsubscribe() const {
        if (!state) {
            std::terminate();
        }
        auto keepAlive = state;
        state->unsubscribe();
    }

    weak_state_type get_weak() {
        return state;
    }

    // Atomically promote weak subscription to strong.
    // Calls std::terminate if w has already expired.
    static subscription lock(weak_state_type w) {
        return subscription(w);
    }

    // Atomically try to promote weak subscription to strong.
    // Returns an empty maybe<> if w has already expired.
    static rxu::maybe<subscription> maybe_lock(weak_state_type w) {
        auto strong_subscription = w.lock();
        if (!strong_subscription) {
            return rxu::detail::maybe<subscription>{};
        } else {
            return rxu::detail::maybe<subscription>{subscription{std::move(strong_subscription)}};
        }
    }
};

inline bool operator<(const subscription& lhs, const subscription& rhs) {
    return lhs.state < rhs.state;
}
inline bool operator==(const subscription& lhs, const subscription& rhs) {
    return lhs.state == rhs.state;
}
inline bool operator!=(const subscription& lhs, const subscription& rhs) {
    return !(lhs == rhs);
}


inline auto make_subscription()
    ->      subscription {
    return  subscription();
}
template<class I>
auto make_subscription(I&& i)
    -> typename std::enable_if<!is_subscription<I>::value && !detail::is_unsubscribe_function<I>::value,
            subscription>::type {
    return  subscription(std::forward<I>(i));
}
template<class Unsubscribe>
auto make_subscription(Unsubscribe&& u)
    -> typename std::enable_if<detail::is_unsubscribe_function<Unsubscribe>::value,
            subscription>::type {
    return  subscription(static_subscription<Unsubscribe>(std::forward<Unsubscribe>(u)));
}

class composite_subscription;

namespace detail {

struct tag_composite_subscription_empty {};

class composite_subscription_inner
{
private:
    using weak_subscription = subscription::weak_state_type;
    struct composite_subscription_state : public std::enable_shared_from_this<composite_subscription_state>
    {
        // invariant: cannot access this data without the lock held.
        std::set<subscription> subscriptions;
        // double checked locking:
        //    issubscribed must be loaded again after each lock acquisition.
        // invariant:
        //    never call subscription::unsubscribe with lock held.
        std::mutex lock;
        // invariant: transitions from 'true' to 'false' exactly once, at any time.
        std::atomic<bool> issubscribed;

        ~composite_subscription_state()
        {
            std::unique_lock<decltype(lock)> guard(lock);
            subscriptions.clear();
        }

        composite_subscription_state()
            : issubscribed(true)
        {
        }
        composite_subscription_state(tag_composite_subscription_empty)
            : issubscribed(false)
        {
        }

        // Atomically add 's' to the set of subscriptions.
        //
        // If unsubscribe() has already occurred, this immediately
        // calls s.unsubscribe().
        //
        // cs.unsubscribe() [must] happens-before s.unsubscribe()
        //
        // Due to the un-atomic nature of calling 's.unsubscribe()',
        // it is possible to observe the unintuitive
        // add(s)=>s.unsubscribe() prior
        // to any of the unsubscribe()=>sN.unsubscribe().
        inline weak_subscription add(subscription s) {
            if (!issubscribed) {  // load.acq [seq_cst]
                s.unsubscribe();
            } else if (s.is_subscribed()) {
                std::unique_lock<decltype(lock)> guard(lock);
                if (!issubscribed) {  // load.acq [seq_cst]
                    // unsubscribe was called concurrently.
                    guard.unlock();
                    // invariant: do not call unsubscribe with lock held.
                    s.unsubscribe();
                } else {
                    subscriptions.insert(s);
                }
            }
            return s.get_weak();
        }

        // Atomically remove 'w' from the set of subscriptions.
        //
        // This does nothing if 'w' was already previously removed,
        // or refers to an expired value.
        inline void remove(weak_subscription w) {
            if (issubscribed) { // load.acq [seq_cst]
                rxu::maybe<subscription> maybe_subscription = subscription::maybe_lock(w);

                if (maybe_subscription.empty()) {
                  // Do nothing if the subscription has already expired.
                  return;
                }

                std::unique_lock<decltype(lock)> guard(lock);
                // invariant: subscriptions must be accessed under the lock.

                if (issubscribed) { // load.acq [seq_cst]
                  subscription& s = maybe_subscription.get();
                  subscriptions.erase(std::move(s));
                } // else unsubscribe() was called concurrently; this becomes a no-op.
            }
        }

        // Atomically clear all subscriptions that were observably added
        // (and not subsequently observably removed).
        //
        // Un-atomically call unsubscribe on those subscriptions.
        //
        // forall subscriptions in {add(s1),add(s2),...}
        //                         - {remove(s3), remove(s4), ...}:
        //   cs.unsubscribe() || cs.clear() happens before s.unsubscribe()
        //
        // cs.unsubscribe() observed-before cs.clear ==> do nothing.
        inline void clear() {
            if (issubscribed) { // load.acq [seq_cst]
                std::unique_lock<decltype(lock)> guard(lock);

                if (!issubscribed) { // load.acq [seq_cst]
                  // unsubscribe was called concurrently.
                  return;
                }

                std::set<subscription> v(std::move(subscriptions));
                // invariant: do not call unsubscribe with lock held.
                guard.unlock();
                std::for_each(v.begin(), v.end(),
                              [](const subscription& s) {
                                s.unsubscribe(); });
            }
        }

        // Atomically clear all subscriptions that were observably added
        // (and not subsequently observably removed).
        //
        // Un-atomically call unsubscribe on those subscriptions.
        //
        // Switches to an 'unsubscribed' state, all subsequent
        // adds are immediately unsubscribed.
        //
        // cs.unsubscribe() [must] happens-before
        //     cs.add(s) ==> s.unsubscribe()
        //
        // forall subscriptions in {add(s1),add(s2),...}
        //                         - {remove(s3), remove(s4), ...}:
        //   cs.unsubscribe() || cs.clear() happens before s.unsubscribe()
        inline void unsubscribe() {
            if (issubscribed.exchange(false)) {  // cas.acq_rel [seq_cst]
                std::unique_lock<decltype(lock)> guard(lock);

                // is_subscribed can only transition to 'false' once,
                // does not need an extra atomic access here.

                std::set<subscription> v(std::move(subscriptions));
                // invariant: do not call unsubscribe with lock held.
                guard.unlock();
                std::for_each(v.begin(), v.end(),
                              [](const subscription& s) {
                                s.unsubscribe(); });
            }
        }
    };

public:
    using shared_state_type = std::shared_ptr<composite_subscription_state>;

protected:
    mutable shared_state_type state;

public:
    composite_subscription_inner()
        : state(std::make_shared<composite_subscription_state>())
    {
    }
    composite_subscription_inner(tag_composite_subscription_empty et)
        : state(std::make_shared<composite_subscription_state>(et))
    {
    }

    composite_subscription_inner(const composite_subscription_inner& o)
        : state(o.state)
    {
        if (!state) {
            std::terminate();
        }
    }
    composite_subscription_inner(composite_subscription_inner&& o)
        : state(std::move(o.state))
    {
        if (!state) {
            std::terminate();
        }
    }

    composite_subscription_inner& operator=(composite_subscription_inner o)
    {
        state = std::move(o.state);
        if (!state) {
            std::terminate();
        }
        return *this;
    }

    inline weak_subscription add(subscription s) const {
        if (!state) {
            std::terminate();
        }
        return state->add(std::move(s));
    }
    inline void remove(weak_subscription w) const {
        if (!state) {
            std::terminate();
        }
        state->remove(std::move(w));
    }
    inline void clear() const {
        if (!state) {
            std::terminate();
        }
        state->clear();
    }
    inline void unsubscribe() {
        if (!state) {
            std::terminate();
        }
        state->unsubscribe();
    }
};

inline composite_subscription shared_empty();

}

/*!
    \brief controls lifetime for scheduler::schedule and observable<T, SourceOperator>::subscribe.

    \ingroup group-core

*/
class composite_subscription
    : protected detail::composite_subscription_inner
    , public subscription
{
    using inner_type = detail::composite_subscription_inner;
public:
    using weak_subscription = subscription::weak_state_type;

    composite_subscription(detail::tag_composite_subscription_empty et)
        : inner_type(et)
        , subscription() // use empty base
    {
    }

public:

    composite_subscription()
        : inner_type()
        , subscription(*static_cast<const inner_type*>(this))
    {
    }

    composite_subscription(const composite_subscription& o)
        : inner_type(o)
        , subscription(static_cast<const subscription&>(o))
    {
    }
    composite_subscription(composite_subscription&& o)
        : inner_type(std::move(o))
        , subscription(std::move(static_cast<subscription&>(o)))
    {
    }

    composite_subscription& operator=(composite_subscription o)
    {
        inner_type::operator=(std::move(o));
        subscription::operator=(std::move(*static_cast<subscription*>(&o)));
        return *this;
    }

    static inline composite_subscription empty() {
        return detail::shared_empty();
    }

    using subscription::is_subscribed;
    using subscription::unsubscribe;

    using inner_type::clear;

    inline weak_subscription add(subscription s) const {
        if (s == static_cast<const subscription&>(*this)) {
            // do not nest the same subscription
            std::terminate();
            //return s.get_weak();
        }
        auto that = this->subscription::state.get();
        trace_activity().subscription_add_enter(*that, s);
        auto w = inner_type::add(std::move(s));
        trace_activity().subscription_add_return(*that);
        return w;
    }

    template<class F>
    auto add(F f) const
    -> typename std::enable_if<detail::is_unsubscribe_function<F>::value, weak_subscription>::type {
        return add(make_subscription(std::move(f)));
    }

    inline void remove(weak_subscription w) const {
        auto that = this->subscription::state.get();
        trace_activity().subscription_remove_enter(*that, w);
        inner_type::remove(w);
        trace_activity().subscription_remove_return(*that);
    }
};

inline bool operator<(const composite_subscription& lhs, const composite_subscription& rhs) {
    return static_cast<const subscription&>(lhs) < static_cast<const subscription&>(rhs);
}
inline bool operator==(const composite_subscription& lhs, const composite_subscription& rhs) {
    return static_cast<const subscription&>(lhs) == static_cast<const subscription&>(rhs);
}
inline bool operator!=(const composite_subscription& lhs, const composite_subscription& rhs) {
    return !(lhs == rhs);
}

namespace detail {

inline composite_subscription shared_empty() {
    static composite_subscription shared_empty = composite_subscription(tag_composite_subscription_empty());
    return shared_empty;
}

}

template<class T>
class resource : public subscription_base
{
public:
    using weak_subscription = typename composite_subscription::weak_subscription;

    resource()
        : lifetime(composite_subscription())
        , value(std::make_shared<rxu::detail::maybe<T>>())
    {
    }

    explicit resource(T t, composite_subscription cs = composite_subscription())
        : lifetime(std::move(cs))
        , value(std::make_shared<rxu::detail::maybe<T>>(rxu::detail::maybe<T>(std::move(t))))
    {
        auto localValue = value;
        lifetime.add(
            [localValue](){
                localValue->reset();
            }
        );
    }

    T& get() {
        return value.get()->get();
    }
    composite_subscription& get_subscription() {
        return lifetime;
    }

    bool is_subscribed() const {
        return lifetime.is_subscribed();
    }
    weak_subscription add(subscription s) const {
        return lifetime.add(std::move(s));
    }
    template<class F>
    auto add(F f) const
    -> typename std::enable_if<detail::is_unsubscribe_function<F>::value, weak_subscription>::type {
        return lifetime.add(make_subscription(std::move(f)));
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

protected:
    composite_subscription lifetime;
    std::shared_ptr<rxu::detail::maybe<T>> value;
};

}

#endif
