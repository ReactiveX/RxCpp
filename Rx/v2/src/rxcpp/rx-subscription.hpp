// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SUBSCRIPTION_HPP)
#define RXCPP_RX_SUBSCRIPTION_HPP

#include "rx-includes.hpp"

namespace rxcpp {


struct tag_subscription {};
struct subscription_base {typedef tag_subscription subscription_tag;};
template<class T>
class is_subscription
{
    template<class C>
    static typename C::subscription_tag* check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<typename std::decay<T>::type>(0)), tag_subscription*>::value;
};

class dynamic_subscription : public subscription_base
{
    typedef std::function<void()> unsubscribe_call_type;
    unsubscribe_call_type unsubscribe_call;
    dynamic_subscription()
    {
    }
public:
    dynamic_subscription(const dynamic_subscription& o)
        : unsubscribe_call(o.unsubscribe_call)
    {
    }
    dynamic_subscription(dynamic_subscription&& o)
        : unsubscribe_call(std::move(o.unsubscribe_call))
    {
    }
    template<class I>
    dynamic_subscription(I i, typename std::enable_if<is_subscription<I>::value && !std::is_same<I, dynamic_subscription>::value, void**>::type selector = nullptr)
        : unsubscribe_call([i](){
            i.unsubscribe();})
    {
    }
    dynamic_subscription(unsubscribe_call_type s)
        : unsubscribe_call(std::move(s))
    {
    }
    void unsubscribe() const {
        unsubscribe_call();
    }
};

template<class Unsubscribe>
class static_subscription : public subscription_base
{
    typedef typename std::decay<Unsubscribe>::type unsubscribe_call_type;
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

template<class I>
class subscription : public subscription_base
{
    typedef typename std::decay<I>::type inner_t;
    inner_t inner;
    mutable bool issubscribed;
public:
    subscription(inner_t inner)
        : inner(std::move(inner))
        , issubscribed(true)
    {
    }
    bool is_subscribed() const {
        return issubscribed;
    }
    void unsubscribe() const {
        if (issubscribed) {
            inner.unsubscribe();
        }
        issubscribed = false;
    }
};
template<>
class subscription<void> : public subscription_base
{
public:
    subscription()
    {
    }
    bool is_subscribed() const {
        return false;
    }
    void unsubscribe() const {
    }
};
inline auto make_subscription()
    ->      subscription<void> {
    return  subscription<void>();
}
template<class I>
auto make_subscription(I&& i)
    -> typename std::enable_if<is_subscription<I>::value,
            subscription<I>>::type {
    return  subscription<I>(std::forward<I>(i));
}
template<class Unsubscribe>
auto make_subscription(Unsubscribe&& u)
    -> typename std::enable_if<!is_subscription<Unsubscribe>::value,
            subscription<   static_subscription<Unsubscribe>>>::type {
    return  subscription<   static_subscription<Unsubscribe>>(
                            static_subscription<Unsubscribe>(std::forward<Unsubscribe>(u)));
}

class composite_subscription : public subscription_base
{
public:
    typedef std::shared_ptr<dynamic_subscription> shared_subscription;
    typedef std::weak_ptr<dynamic_subscription> weak_subscription;
private:
    struct tag_empty {};
    struct state_t : public std::enable_shared_from_this<state_t>
    {
        std::vector<shared_subscription> subscriptions;
        std::recursive_mutex lock;
        bool issubscribed;

        state_t()
            : issubscribed(true)
        {
        }

        state_t(tag_empty&&)
            : issubscribed(false)
        {
        }

        inline bool is_subscribed() {
            return issubscribed;
        }

        inline weak_subscription add(dynamic_subscription s) {
            return add(std::make_shared<dynamic_subscription>(std::move(s)));
        }

        inline weak_subscription add(shared_subscription s) {
            std::unique_lock<decltype(lock)> guard(lock);

            if (!issubscribed) {
                s->unsubscribe();
            } else {
                auto end = std::end(subscriptions);
                auto it = std::find(std::begin(subscriptions), end, s);
                if (it == end)
                {
                    subscriptions.emplace_back(s);
                }
            }
            return s;
        }

        inline void remove(weak_subscription w) {
            std::unique_lock<decltype(lock)> guard(lock);

            if (issubscribed && !w.expired()) {
                auto s = w.lock();
                if (s)
                {
                    auto end = std::end(subscriptions);
                    auto it = std::find(std::begin(subscriptions), end, s);
                    if (it != end)
                    {
                        subscriptions.erase(it);
                    }
                }
            }
        }

        inline void clear() {
            std::unique_lock<decltype(lock)> guard(lock);

            if (issubscribed) {
                std::vector<shared_subscription> v(std::move(subscriptions));
                std::for_each(v.begin(), v.end(),
                              [](shared_subscription& s) {
                                s->unsubscribe(); });
            }
        }

        inline void unsubscribe() {
            std::unique_lock<decltype(lock)> guard(lock);

            if (issubscribed) {
                issubscribed = false;
                std::vector<shared_subscription> v(std::move(subscriptions));
                std::for_each(v.begin(), v.end(),
                              [](shared_subscription& s) {
                                s->unsubscribe(); });
            }
        }
    };

    mutable std::shared_ptr<state_t> state;

    static std::shared_ptr<state_t> shared_empty;

    composite_subscription(std::shared_ptr<state_t> s)
        : state(std::move(s))
    {
        if (!state) {
            abort();
        }
    }

    friend bool operator==(const composite_subscription&, const composite_subscription&);

public:

    composite_subscription()
        : state(std::make_shared<state_t>())
    {
        if (!state) {
            abort();
        }
    }
    composite_subscription(const composite_subscription& o)
        : state(o.state)
    {
        if (!state) {
            abort();
        }
    }
    composite_subscription(composite_subscription&& o)
        : state(std::move(o.state))
    {
        if (!state) {
            abort();
        }
    }

    composite_subscription& operator=(const composite_subscription& o)
    {
        state = o.state;
        if (!state) {
            abort();
        }
        return *this;
    }
    composite_subscription& operator=(composite_subscription&& o)
    {
        state = std::move(o.state);
        if (!state) {
            abort();
        }
        return *this;
    }

    static inline composite_subscription empty() {
        return composite_subscription(shared_empty);
    }

    inline bool is_subscribed() const {
        return state->is_subscribed();
    }
    inline weak_subscription add(shared_subscription s) const {
        return state->add(std::move(s));
    }
    inline weak_subscription add(dynamic_subscription s) const {
        return state->add(std::move(s));
    }
    inline void remove(weak_subscription w) const {
        state->remove(std::move(w));
    }
    inline void clear() const {
        state->clear();
    }
    inline void unsubscribe() const {
        state->unsubscribe();
    }
};

inline bool operator==(const composite_subscription& lhs, const composite_subscription& rhs) {
    return lhs.state == rhs.state;
}
inline bool operator!=(const composite_subscription& lhs, const composite_subscription& rhs) {
    return !(lhs == rhs);
}

//static
RXCPP_SELECT_ANY std::shared_ptr<composite_subscription::state_t> composite_subscription::shared_empty = std::make_shared<composite_subscription::state_t>(composite_subscription::tag_empty());


namespace detail {

struct tag_subscription_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = !is_subscriber<LHS>::value && !is_observer<LHS>::value && is_subscription<LHS>::value;
    };
    struct default_type {
        inline operator composite_subscription() const {
            return composite_subscription();
        }
    };
};

}

}

#endif
