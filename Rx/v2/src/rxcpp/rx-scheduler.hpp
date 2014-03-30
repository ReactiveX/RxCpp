// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_HPP)
#define RXCPP_RX_SCHEDULER_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace schedulers {

class scheduler_interface;

namespace detail {

class action_type;
typedef std::shared_ptr<action_type> action_ptr;

typedef std::shared_ptr<scheduler_interface> scheduler_interface_ptr;
typedef std::shared_ptr<const scheduler_interface> const_scheduler_interface_ptr;

}

// It is essential to keep virtual function calls out of an inner loop.
// To make tail-recursion work efficiently the recursion objects create
// a space on the stack inside the virtual function call in the actor that
// allows the callback and the scheduler to share stack space that records
// the request and the allowance without any virtual calls in the loop.

class recursed
{
    bool& isrequested;
public:
    explicit recursed(bool& r)
        : isrequested(r)
    {
    }
    inline void operator()() const {
        isrequested = true;
    }
};

class recurse
{
    bool& isallowed;
    mutable bool isrequested;
    recursed requestor;
public:
    explicit recurse(bool& a)
        : isallowed(a)
        , isrequested(true)
        , requestor(isrequested)
    {
    }
    inline bool is_allowed() const {
        return isallowed;
    }
    inline bool is_requested() const {
        return isrequested;
    }
    inline void reset() const {
        isrequested = false;
    }
    inline const recursed& get_recursed() const {
        return requestor;
    }
};

class recursion
{
    mutable bool isallowed;
    recurse recursor;
public:
    recursion()
        : isallowed(true)
        , recursor(isallowed)
    {
    }
    explicit recursion(bool b)
        : isallowed(b)
        , recursor(isallowed)
    {
    }
    inline void reset(bool b = true) const {
        isallowed = b;
    }
    inline const recurse& get_recurse() const {
        return recursor;
    }
};


struct action_base
{
    typedef tag_action action_tag;
};

class schedulable;

namespace action_duration {
    enum type {
        invalid,
        runs_short,
        runs_long
    };
}

class action : public action_base
{
    typedef action this_type;
    detail::action_ptr inner;
    static detail::action_ptr shared_empty;
    friend bool operator==(const action&, const action&);
public:
    action()
    {
    }
    explicit action(detail::action_ptr i)
    : inner(std::move(i))
    {
    }

    inline static action empty() {
        return action(shared_empty);
    }

    inline action_duration::type get_duration() const;

    inline void operator()(const schedulable& s, const recurse& r) const;
};

struct scheduler_base
{
    typedef std::chrono::steady_clock clock_type;
    typedef tag_scheduler scheduler_tag;
};

class schedulable;

class scheduler_interface
    : public std::enable_shared_from_this<scheduler_interface>
{
    typedef scheduler_interface this_type;

public:
    typedef scheduler_base::clock_type clock_type;

    virtual ~scheduler_interface() {}

    virtual clock_type::time_point now() const = 0;

    virtual void schedule(const schedulable& scbl) const = 0;
    virtual void schedule(clock_type::duration when, const schedulable& scbl) const = 0;
    virtual void schedule(clock_type::time_point when, const schedulable& scbl) const = 0;
};


struct schedulable_base : public subscription_base, public scheduler_base, public action_base
{
    typedef tag_schedulable schedulable_tag;
};

inline bool operator==(const action& lhs, const action& rhs) {
    return lhs.inner == rhs.inner;
}
inline bool operator!=(const action& lhs, const action& rhs) {
    return !(lhs == rhs);
}

namespace detail {

template<class F>
struct is_action_function
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)(*(schedulable*)nullptr));
    template<class CF>
    static not_void check(...);

    static const bool value = std::is_same<decltype(check<typename std::decay<F>::type>(0)), void>::value;
};

}

class scheduler : public scheduler_base
{
    typedef scheduler this_type;
    detail::scheduler_interface_ptr inner;
    friend bool operator==(const scheduler&, const scheduler&);
public:
    typedef scheduler_base::clock_type clock_type;

    scheduler()
    {
    }
    explicit scheduler(detail::scheduler_interface_ptr i)
        : inner(std::move(i))
    {
    }
    explicit scheduler(detail::const_scheduler_interface_ptr i)
        : inner(std::const_pointer_cast<scheduler_interface>(i))
    {
    }

    inline clock_type::time_point now() const {
        return inner->now();
    }

    inline void schedule(const schedulable& scbl) const {
        inner->schedule(scbl);
    }
    inline void schedule(clock_type::duration when, const schedulable& scbl) const {
        inner->schedule(when, scbl);
    }
    inline void schedule(clock_type::time_point when, const schedulable& scbl) const {
        inner->schedule(when, scbl);
    }

    template<class Arg0, class... ArgN>
    auto schedule(Arg0&& a0, ArgN&&... an) const
        -> typename std::enable_if<
            (detail::is_action_function<Arg0>::value ||
            is_subscription<Arg0>::value) &&
            !is_schedulable<Arg0>::value>::type;
    template<class Arg0, class... ArgN>
    auto schedule(Arg0&& a0, ArgN&&... an) const
        -> typename std::enable_if<
            !detail::is_action_function<Arg0>::value &&
            !is_subscription<Arg0>::value &&
            !is_scheduler<Arg0>::value &&
            !is_schedulable<Arg0>::value>::type;
};

inline bool operator==(const scheduler& lhs, const scheduler& rhs) {
    return lhs.inner == rhs.inner;
}
inline bool operator!=(const scheduler& lhs, const scheduler& rhs) {
    return !(lhs == rhs);
}

template<class Scheduler>
inline scheduler make_scheduler() {
    return scheduler(std::static_pointer_cast<scheduler_interface>(std::make_shared<Scheduler>()));
}


class schedulable : public schedulable_base
{
    typedef schedulable this_type;

    composite_subscription lifetime;
    scheduler controller;
    action activity;

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

    class recursed_scope_type
    {
        mutable const recursed* requestor;

        class exit_recursed_scope_type
        {
            const recursed_scope_type* that;
        public:
            ~exit_recursed_scope_type()
            {
                    that->requestor = nullptr;
            }
            exit_recursed_scope_type(const recursed_scope_type* that)
                : that(that)
            {
            }
        };
    public:
        recursed_scope_type()
            : requestor(nullptr)
        {
        }
        recursed_scope_type(const recursed_scope_type&)
            : requestor(nullptr)
        {
            // does not aquire recursion scope
        }
        recursed_scope_type& operator=(const recursed_scope_type& o)
        {
            // no change in recursion scope
            return *this;
        }
        exit_recursed_scope_type reset(const recurse& r) const {
            requestor = std::addressof(r.get_recursed());
            return exit_recursed_scope_type(this);
        }
        bool is_recursed() const {
            return !!requestor;
        }
        void operator()() const {
            (*requestor)();
        }
    };
    recursed_scope_type recursed_scope;

public:
    typedef composite_subscription::weak_subscription weak_subscription;
    typedef composite_subscription::shared_subscription shared_subscription;
    typedef scheduler_base::clock_type clock_type;

    schedulable()
    {
    }
    schedulable(composite_subscription cs, scheduler q, action a)
        : lifetime(std::move(cs))
        , controller(std::move(q))
        , activity(std::move(a))
    {
    }

    inline const composite_subscription& get_subscription() const {
        return lifetime;
    }
    inline composite_subscription& get_subscription() {
        return lifetime;
    }
    inline const scheduler& get_scheduler() const {
        return controller;
    }
    inline scheduler& get_scheduler() {
        return controller;
    }
    inline const action& get_action() const {
        return activity;
    }
    inline action& get_action() {
        return activity;
    }

    inline static schedulable empty(scheduler sc) {
        return schedulable(composite_subscription::empty(), sc, action::empty());
    }

    inline auto set_recursed(const recurse& r) const
        -> decltype(recursed_scope.reset(r)) {
        return      recursed_scope.reset(r);
    }

    // recursed
    //
    bool is_recursed() const {
        return recursed_scope.is_recursed();
    }
    /// requests tail-recursion of the same action
    /// this will exit the process if called when
    /// is_recursed() is false.
    /// Note: to improve perf it is not required
    /// to call is_recursed() before calling this
    /// operator. Context is sufficient. The schedulable
    /// passed to the action by the scheduler will return
    /// true from is_recursed()
    inline void operator()() const {
        recursed_scope();
    }

    // composite_subscription
    //
    inline bool is_subscribed() const {
        return lifetime.is_subscribed();
    }
    inline weak_subscription add(shared_subscription s) const {
        return lifetime.add(std::move(s));
    }
    inline weak_subscription add(dynamic_subscription s) const {
        return lifetime.add(std::move(s));
    }
    inline void remove(weak_subscription w) const {
        return lifetime.remove(std::move(w));
    }
    inline void clear() const {
        return lifetime.clear();
    }
    inline void unsubscribe() const {
        return lifetime.unsubscribe();
    }

    // scheduler
    //
    inline clock_type::time_point now() const {
        return controller.now();
    }
    /// put this on the queue of the stored scheduler to run asap
    inline void schedule() const {
        controller.schedule(*this);
    }
    /// put this on the queue of the stored scheduler to run after a delay from now
    inline void schedule(clock_type::duration when) const {
        controller.schedule(when, *this);
    }
    /// put this on the queue of the stored scheduler to run at the specified time
    inline void schedule(clock_type::time_point when) const {
        controller.schedule(when, *this);
    }

    // action
    //
    /// some schedulers care about how long an action will run
    /// this is how an action declares its behavior
    inline action_duration::type get_duration() const {
        return activity.get_duration();
    }
    ///
    inline void operator()(const recurse& r) const {
        if (!is_subscribed()) {
            abort();
        }
        detacher protect(this);
        activity(*this, r);
        protect.that = nullptr;
    }
};

inline bool operator==(const schedulable& lhs, const schedulable& rhs) {
    return  lhs.get_action() == rhs.get_action() &&
            lhs.get_scheduler() == rhs.get_scheduler() &&
            lhs.get_subscription() == rhs.get_subscription();
}
inline bool operator!=(const schedulable& lhs, const schedulable& rhs) {
    return !(lhs == rhs);
}

struct current_thread;

namespace detail {

class action_type
    : public std::enable_shared_from_this<action_type>
{
    typedef action_type this_type;

public:
    typedef std::function<void(const schedulable&, const recurse&)> function_type;

private:
    action_duration::type d;
    function_type f;

public:
    action_type()
    {
    }

    action_type(action_duration::type d, function_type f)
        : d(d)
        , f(std::move(f))
    {
    }

    inline action_duration::type get_duration() const {
        return d;
    }

    inline void operator()(const schedulable& s, const recurse& r) {
        if (!f) {
            abort();
        }
        f(s, r);
    }
};

}


inline action_duration::type action::get_duration() const {
    return inner->get_duration();
}

inline void action::operator()(const schedulable& s, const recurse& r) const {
    (*inner)(s, r);
}

//static
RXCPP_SELECT_ANY detail::action_ptr action::shared_empty = detail::action_ptr(new detail::action_type());


inline action make_action_empty() {
    return action::empty();
}

template<class F>
inline action make_action(F&& f, action_duration::type d = action_duration::runs_short) {
    static_assert(detail::is_action_function<F>::value, "action function must be void(schedulable)");
    auto fn = std::forward<F>(f);
    return action(std::make_shared<detail::action_type>(
        d,
        // tail-recurse inside of the virtual function call
        // until a new action, lifetime or scheduler is returned
        [fn](const schedulable& s, const recurse& r) {
            auto scope = s.set_recursed(r);
            while (s.is_subscribed()) {
                r.reset();
                fn(s);
                if (!r.is_allowed() || !r.is_requested()) {
                    if (r.is_requested()) {
                        s.schedule();
                    }
                    break;
                }
            }
        }));
}

// copy
inline auto make_schedulable(
    const   schedulable& scbl)
    ->      schedulable {
    return  schedulable(scbl);
}
// move
inline auto make_schedulable(
            schedulable&& scbl)
    ->      schedulable {
    return  schedulable(std::move(scbl));
}

// action
//

template<class F>
auto make_schedulable(scheduler sc, F&& f, action_duration::type d = action_duration::runs_short)
    -> typename std::enable_if<detail::is_action_function<F>::value, schedulable>::type {
    return schedulable(composite_subscription(), sc, make_action(std::forward<F>(f), d));
}
template<class F>
auto make_schedulable(scheduler sc, composite_subscription cs, F&& f, action_duration::type d = action_duration::runs_short)
    -> typename std::enable_if<detail::is_action_function<F>::value, schedulable>::type {
    return schedulable(cs, sc, make_action(std::forward<F>(f), d));
}
template<class F>
auto make_schedulable(schedulable scbl, composite_subscription cs, F&& f, action_duration::type d = action_duration::runs_short)
    -> typename std::enable_if<detail::is_action_function<F>::value, schedulable>::type {
    return schedulable(cs, scbl.get_scheduler(), make_action(std::forward<F>(f), d));
}
template<class F>
auto make_schedulable(schedulable scbl, scheduler sc, F&& f, action_duration::type d = action_duration::runs_short)
    -> typename std::enable_if<detail::is_action_function<F>::value, schedulable>::type {
    return schedulable(scbl.get_subscription(), sc, make_action(std::forward<F>(f), d));
}
template<class F>
auto make_schedulable(schedulable scbl, F&& f, action_duration::type d = action_duration::runs_short)
    -> typename std::enable_if<detail::is_action_function<F>::value, schedulable>::type {
    return schedulable(scbl.get_subscription(), scbl.get_scheduler(), make_action(std::forward<F>(f), d));
}

inline auto make_schedulable(schedulable scbl, composite_subscription cs)
    -> schedulable {
    return schedulable(cs, scbl.get_scheduler(), scbl.get_action());
}
inline auto make_schedulable(schedulable scbl, scheduler sc, composite_subscription cs)
    -> schedulable {
    return schedulable(cs, sc, scbl.get_action());
}
inline auto make_schedulable(schedulable scbl, scheduler sc)
    -> schedulable {
    return schedulable(composite_subscription(), sc, scbl.get_action());
}

template<class Arg0, class... ArgN>
auto scheduler::schedule(Arg0&& a0, ArgN&&... an) const
    -> typename std::enable_if<
        (detail::is_action_function<Arg0>::value ||
        is_subscription<Arg0>::value) &&
        !is_schedulable<Arg0>::value>::type {
    return this->schedule(make_schedulable(*this, std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
}
template<class Arg0, class... ArgN>
auto scheduler::schedule(Arg0&& a0, ArgN&&... an) const
    -> typename std::enable_if<
        !detail::is_action_function<Arg0>::value &&
        !is_subscription<Arg0>::value &&
        !is_scheduler<Arg0>::value &&
        !is_schedulable<Arg0>::value>::type {
    return this->schedule(std::forward<Arg0>(a0), make_schedulable(*this, std::forward<ArgN>(an)...));
}

namespace detail {

template<class TimePoint>
struct time_schedulable
{
    time_schedulable(TimePoint when, schedulable a)
        : when(when)
        , what(std::move(a))
    {
    }
    TimePoint when;
    schedulable what;
};

}

}
namespace rxsc=schedulers;

}

#include "schedulers/rx-currentthread.hpp"
#include "schedulers/rx-virtualtime.hpp"

#endif
