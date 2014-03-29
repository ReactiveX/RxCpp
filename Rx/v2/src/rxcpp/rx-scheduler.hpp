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

struct tag_action_function_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = is_action_function<LHS>::value;
    };
    typedef detail::action_type::function_type default_type;
};

struct tag_action_duration_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = std::is_same<typename std::decay<LHS>::type, action_duration::type>::value;
    };
    struct default_type {
        inline operator action_duration::type() const {
            return action_duration::runs_short;
        }
    };
};

struct tag_when_resolution
{
    typedef scheduler_interface::clock_type clock_type;
    typedef clock_type::duration duration_type;
    typedef clock_type::time_point time_point_type;
    template<class LHS>
    struct predicate
    {
        typedef typename std::decay<LHS>::type decayedlhs;
        static const bool value =   std::is_same<decayedlhs, time_point_type>::value ||
                                    std::is_same<decayedlhs, duration_type>::value;
    };
    struct default_type {
        inline operator time_point_type() const {
            return clock_type::now();
        }
    };
};

struct tag_schedulable_resolution
{
    template<class LHS>
    struct predicate : public is_schedulable<LHS>
    {
    };
    typedef schedulable default_type;
};

struct tag_action_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = !is_schedulable<LHS>::value && is_action<LHS>::value;
    };
    typedef action default_type;
};

struct tag_scheduler_resolution
{
    template<class LHS>
    struct predicate
    {
        static const bool value = !is_schedulable<LHS>::value && is_scheduler<LHS>::value;
    };
    typedef scheduler default_type;
};


typedef rxu::detail::tag_set<tag_when_resolution,
            tag_schedulable_resolution,
            rxcpp::detail::tag_subscription_resolution,
            tag_scheduler_resolution,
            tag_action_resolution,
            tag_action_function_resolution,
            tag_action_duration_resolution> tag_schedulable_set;

template<bool schedulable_is_arg, bool action_is_arg, bool action_function_is_arg>
struct action_selector;

template<bool schedulable_is_arg>
struct action_selector<schedulable_is_arg, true, false>
{
    template<class Set>
    static action get_action(Set& rs) {
        return std::get<4>(rs).value;
    }
};
template<bool schedulable_is_arg>
struct action_selector<schedulable_is_arg, false, true>
{
    template<class Set>
    static action get_action(Set& rs) {
        return make_action(std::get<5>(rs).value, std::get<6>(rs).value);
    }
};
template<>
struct action_selector<true, false, false>
{
    template<class Set>
    static action get_action(Set& rs) {
        return std::get<1>(rs).value.get_action();
    }
};

template<class ResolvedArgSet>
action select_action(ResolvedArgSet& rs) {
    return action_selector<std::decay<decltype(std::get<1>(rs))>::type::is_arg, std::decay<decltype(std::get<4>(rs))>::type::is_arg, std::decay<decltype(std::get<5>(rs))>::type::is_arg>::get_action(rs);

    typedef typename std::decay<decltype(std::get<3>(std::forward<ResolvedArgSet>(rs)))>::type rsc_t;
    typedef typename std::decay<decltype(std::get<5>(std::forward<ResolvedArgSet>(rs)))>::type raf_t;
    typedef typename std::decay<decltype(std::get<6>(std::forward<ResolvedArgSet>(rs)))>::type rad_t;
    typedef typename std::decay<decltype(std::get<4>(std::forward<ResolvedArgSet>(rs)))>::type ra_t;
    typedef typename std::decay<decltype(std::get<1>(std::forward<ResolvedArgSet>(rs)))>::type rscbl_t;

    static_assert(rscbl_t::is_arg || ra_t::is_arg || raf_t::is_arg, "at least one of; action_function, action or schedulable is required");
    static_assert(int(ra_t::is_arg) + int(raf_t::is_arg) < 2, "action_function not allowed with an action");
    static_assert(int(ra_t::is_arg) + int(rad_t::is_arg) < 2, "action_duration not allowed with an action");
}

template<bool when_invalid, class ResolvedArgSet>
schedulable make_schedulable_resolved(ResolvedArgSet&& rsArg) {
    const auto rs = std::forward<ResolvedArgSet>(rsArg);
    const auto rsub = std::get<2>(rs);
    const auto rsc = std::get<3>(rs);
    const auto rscbl = std::get<1>(rs);
    const auto sc =     (rscbl.is_arg && !rsc.is_arg)   ? rscbl.value.get_scheduler()      : rsc.value;
    const auto sub =    (rscbl.is_arg && !rsub.is_arg)  ? rscbl.value.get_subscription()   : rsub.value;
    return  schedulable(sub, sc, select_action(rs));

    typedef typename std::decay<decltype(std::get<0>(rs))>::type rw_t;
    typedef typename std::decay<decltype(std::get<3>(rs))>::type rsc_t;
    typedef typename std::decay<decltype(std::get<1>(rs))>::type rscbl_t;

    static_assert(when_invalid || !rw_t::is_arg, "when is an invalid parameter");
    static_assert(rscbl_t::is_arg || rsc_t::is_arg, "at least one of; scheduler or schedulable is required");
}

template<class ResolvedArgSet>
schedulable schedule_resolved(ResolvedArgSet&& rsArg) {
    const auto rw = std::get<0>(rsArg);
    schedulable result = make_schedulable_resolved<false>(std::forward<ResolvedArgSet>(rsArg));
    if (rw.is_arg) {
        result.schedule(rw.value);
    } else {
        result.schedule();
    }
    return result;
}

}

template<class Arg0, class... ArgN>
schedulable make_schedulable(Arg0&& a0, ArgN&&... an) {
    return detail::make_schedulable_resolved<true>(rxu::detail::resolve_arg_set(detail::tag_schedulable_set(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
}

template<class Arg0, class... ArgN>
schedulable schedule(Arg0&& a0, ArgN&&... an) {
    return detail::schedule_resolved(rxu::detail::resolve_arg_set(detail::tag_schedulable_set(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
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
