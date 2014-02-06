// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_HPP)
#define RXCPP_RX_HPP

#include <memory>
#include <utility>
#include <algorithm>
#include <exception>
#include <functional>
#include <vector>
#include <mutex>

namespace rxcpp {

    struct tag_subscription {};
    struct subscription_base {typedef tag_subscription subscription_tag;};
    template<class T>
    class is_subscription
    {
        template<class C>
        static typename C::subscription_tag check(int);
        template<class C>
        static void check(...);
    public:
        static const bool value = std::is_convertible<decltype(check<T>(0)), tag_subscription>::value;
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
        typedef Unsubscribe unsubscribe_call_type;
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
        typedef I inner_t;
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
    auto make_subscription(I i)
        -> typename std::enable_if<is_subscription<I>::value,
                subscription<I>>::type {
        return  subscription<I>(std::move(i));
    }
    template<class Unsubscribe>
    auto make_subscription(Unsubscribe u)
        -> typename std::enable_if<!is_subscription<Unsubscribe>::value,
                subscription<   static_subscription<Unsubscribe>>>::type {
        return  subscription<   static_subscription<Unsubscribe>>(
                                static_subscription<Unsubscribe>(std::move(u)));
    }

    class composite_subscription : public subscription_base
    {
    public:
        typedef std::shared_ptr<dynamic_subscription> shared_subscription;
        typedef std::weak_ptr<dynamic_subscription> weak_subscription;
    private:
        struct state_t
        {
            std::vector<shared_subscription> subscriptions;
            std::mutex lock;
            bool issubscribed;

            state_t()
                : issubscribed(true)
            {
            }

            bool is_subscribed() {
                return issubscribed;
            }

            weak_subscription add(dynamic_subscription s) {
                return add(std::make_shared<dynamic_subscription>(std::move(s)));
            }

            weak_subscription add(shared_subscription s) {
                std::unique_lock<decltype(lock)> guard(lock);

                if (!issubscribed) {
                    s->unsubscribe();
                } else {
                    auto end = std::end(subscriptions);
                    auto it = std::find(std::begin(subscriptions), end, s);
                    if (it == end)
                    {
                        subscriptions.emplace_back(std::move(s));
                    }
                }
                return s;
            }

            void remove(weak_subscription w) {
                std::unique_lock<decltype(lock)> guard(lock);

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

            void clear() {
                std::unique_lock<decltype(lock)> guard(lock);

                if (issubscribed) {
                    std::vector<shared_subscription> v(std::move(subscriptions));
                    std::for_each(v.begin(), v.end(),
                                  [](shared_subscription& s) {
                                    s->unsubscribe(); });
                }
            }

            void unsubscribe() {
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

    public:

        composite_subscription()
            : state(std::make_shared<state_t>())
        {
        }
        composite_subscription(const composite_subscription& o)
            : state(o.state)
        {
        }
        composite_subscription(composite_subscription&& o)
            : state(std::move(o.state))
        {
        }

        composite_subscription& operator=(const composite_subscription& o)
        {
            state = o.state;
            return *this;
        }
        composite_subscription& operator=(composite_subscription&& o)
        {
            state = std::move(o.state);
            return *this;
        }

        bool is_subscribed() const {
            return state->is_subscribed();
        }
        weak_subscription add(shared_subscription s) const {
            return state->add(std::move(s));
        }
        weak_subscription add(dynamic_subscription s) const {
            return state->add(std::move(s));
        }
        void remove(weak_subscription w) const {
            state->remove(w);
        }
        void clear() const {
            state->clear();
        }
        void unsubscribe() const {
            state->unsubscribe();
        }
    };

    struct tag_observer {};
    template<class T>
    struct observer_root : public subscription_base
    {
        typedef T value_type;
        typedef composite_subscription subscription_type;
        typedef typename subscription_type::weak_subscription weak_subscription;
        typedef tag_observer observer_tag;
    };
    template<class T, class SelectVoid>
    struct observer_base : public observer_root<T>
    {
    private:
        typedef observer_base this_type;

        mutable typename this_type::subscription_type s;

        observer_base();
    public:
        observer_base(typename this_type::subscription_type s)
            : s(std::move(s))
        {
        }
        observer_base(const observer_base& o)
            : s(o.s)
        {
        }
        observer_base(observer_base&& o)
            : s(std::move(o.s))
        {
        }
        observer_base& operator=(observer_base o) {
            swap(o);
            return *this;
        }
        void swap(observer_base& o) {
            using std::swap;
            swap(s, o.s);
        }
        bool is_subscribed() const {
            return s.is_subscribed();
        }
        typename this_type::weak_subscription add(dynamic_subscription ds) const {
            return s.add(std::move(ds));
        }
        void remove(typename this_type::weak_subscription ws) const {
            s.remove(ws);
        }
        void unsubscribe() const {
            s.unsubscribe();
        }
    };
    template<class T>
    struct observer_base<T, void> : public observer_root<T>
    {
    private:
        typedef observer_base this_type;

    public:
        void swap(observer_base&) {
        }
        bool is_subscribed() const {
            return false;
        }
        typename this_type::weak_subscription add(dynamic_subscription ds) const {
            ds.unsubscribe();
            return typename this_type::weak_subscription();
        }
        void remove(typename this_type::weak_subscription) const {
        }
        void unsubscribe() const {
        }
    };
    template<class T>
    class is_observer
    {
        template<class C>
        static typename C::observer_tag check(int);
        template<class C>
        static void check(...);
    public:
        static const bool value = std::is_convertible<decltype(check<T>(0)), tag_observer>::value;
    };

namespace detail {
    struct OnErrorEmpty
    {
        void operator()(std::exception_ptr) const {}
    };
    struct OnCompletedEmpty
    {
        void operator()() const {}
    };
}

    template<class T>
    class dynamic_observer : public observer_base<T, int>
    {
    public:
        typedef std::function<void(T)> on_next_t;
        typedef std::function<void(std::exception_ptr)> on_error_t;
        typedef std::function<void()> on_completed_t;

    private:
        typedef observer_base<T, int> base_type;

        on_next_t onnext;
        on_error_t onerror;
        on_completed_t oncompleted;

    public:
        dynamic_observer()
        {
        }
        dynamic_observer(composite_subscription cs, on_next_t n = nullptr, on_error_t e = nullptr, on_completed_t c = nullptr)
            : base_type(std::move(cs))
            , onnext(std::move(n))
            , onerror(std::move(e))
            , oncompleted(std::move(c))
        {
        }
        dynamic_observer(on_next_t n, on_error_t e = nullptr, on_completed_t c = nullptr)
            : base_type(composite_subscription())
            , onnext(std::move(n))
            , onerror(std::move(e))
            , oncompleted(std::move(c))
        {
        }
        dynamic_observer(const dynamic_observer& o)
            : base_type(o)
            , onnext(o.onnext)
            , onerror(o.onerror)
            , oncompleted(o.oncompleted)
        {
        }
        dynamic_observer(dynamic_observer&& o)
            : base_type(std::move(o))
            , onnext(std::move(o.onnext))
            , onerror(std::move(o.onerror))
            , oncompleted(std::move(o.oncompleted))
        {
        }
        dynamic_observer& operator=(dynamic_observer o) {
            swap(o);
            return *this;
        }
        void swap(dynamic_observer& o) {
            using std::swap;
            observer_base<T, int>::swap(o);
            swap(onnext, o.onnext);
            swap(onerror, o.onerror);
            swap(oncompleted, o.oncompleted);
        }

        void on_next(T t) const {
            if (onnext) {
                onnext(std::move(t));
            }
        }
        void on_error(std::exception_ptr e) const {
            if (onerror) {
                onerror(e);
            }
        }
        void on_completed() const {
            if (oncompleted) {
                oncompleted();
            }
        }
    };

    template<class T, class OnNext, class OnError, class OnCompleted>
    class static_observer : public observer_base<T, int>
    {
    public:
        typedef OnNext on_next_t;
        typedef OnError on_error_t;
        typedef OnCompleted on_completed_t;

    private:
        on_next_t onnext;
        on_error_t onerror;
        on_completed_t oncompleted;

    public:
        static_observer()
        {
        }
        static_observer(composite_subscription cs, on_next_t n = nullptr, on_error_t e = nullptr, on_completed_t c = nullptr)
            : observer_base<T, int>(std::move(cs))
            , onnext(std::move(n))
            , onerror(std::move(e))
            , oncompleted(std::move(c))
        {
        }
        static_observer(on_next_t n, on_error_t e = nullptr, on_completed_t c = nullptr)
            : observer_base<T, int>(composite_subscription())
            , onnext(std::move(n))
            , onerror(std::move(e))
            , oncompleted(std::move(c))
        {
        }
        static_observer(const static_observer& o)
            : observer_base<T, int>(o)
            , onnext(o.onnext)
            , onerror(o.onerror)
            , oncompleted(o.oncompleted)
        {
        }
        static_observer(static_observer&& o)
            : observer_base<T, int>(std::move(o))
            , onnext(std::move(o.onnext))
            , onerror(std::move(o.onerror))
            , oncompleted(std::move(o.oncompleted))
        {
        }
        static_observer& operator=(static_observer o) {
            swap(o);
            return *this;
        }
        void swap(static_observer& o) {
            using std::swap;
            observer_base<T, int>::swap(o);
            swap(onnext, o.onnext);
            swap(onerror, o.onerror);
            swap(oncompleted, o.oncompleted);
        }

        void on_next(T t) const {
            onnext(std::move(t));
        }
        void on_error(std::exception_ptr e) const {
            onerror(e);
        }
        void on_completed() const {
            oncompleted();
        }
    };

    template<class T, class I>
    class observer : public observer_root<T>
    {
        typedef observer this_type;
        typedef typename std::conditional<is_observer<I>::value, I, dynamic_observer<T>>::type

        inner_t;
        inner_t inner;
    public:
        ~observer()
        {
        }
        observer(inner_t inner)
            : inner(std::move(inner))
        {
        }
        void on_next(T t) const {
            if (is_subscribed()) {
                inner.on_next(std::move(t));
            }
        }
        void on_error(std::exception_ptr e) const {
            if (is_subscribed()) {
                inner.on_error(e);
            }
            unsubscribe();
        }
        void on_completed() const {
            if (is_subscribed()) {
                inner.on_completed();
            }
            unsubscribe();
        }
        bool is_subscribed() const {
            return inner.is_subscribed();
        }
        typename this_type::weak_subscription add(dynamic_subscription ds) const {
            return inner.add(std::move(ds));
        }
        void remove(typename this_type::weak_subscription ws) const {
            inner.remove(ws);
        }
        void unsubscribe() const {
            inner.unsubscribe();
        }
    };
    template<class T>
    class observer<T, void> : public observer_base<T, void>
    {
    public:
        observer()
        {
        }
        void on_next(T&&) const {
        }
        void on_error(std::exception_ptr) const {
        }
        void on_completed() const {
        }
    };
    template<class T>
    auto make_observer()
        ->     observer<T, void> {
        return observer<T, void>();
    }
    template<class I>
    auto make_observer(I i)
        -> typename std::enable_if<is_observer<I>::value, observer<typename I::value_type, I>>::type {
        return                                            observer<typename I::value_type, I>(std::move(i));
    }
    template<class T>
    auto make_observer(typename dynamic_observer<T>::on_next_t n, typename dynamic_observer<T>::on_error_t e = nullptr, typename dynamic_observer<T>::on_completed_t c = nullptr)
        ->      observer<T, dynamic_observer<T>> {
        return  observer<T, dynamic_observer<T>>(dynamic_observer<T>(std::move(n), std::move(e), std::move(c)));
    }
    template<class T, class OnNext, class OnError, class OnCompleted>
    auto make_observer(OnNext n, OnError e, OnCompleted c)
        ->      observer<T, static_observer<T, OnNext, OnError, OnCompleted>> {
        return  observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                            static_observer<T, OnNext, OnError, OnCompleted>(std::move(n), std::move(e), std::move(c)));
    }
    template<class T, class OnNext, class OnError>
    auto make_observer(OnNext n, OnError e)
        ->      observer<T, static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>> {
        return  observer<T, static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>>(
                            static_observer<T, OnNext, OnError, detail::OnCompletedEmpty>(std::move(n), std::move(e), detail::OnCompletedEmpty()));
    }
    template<class T, class OnNext>
    auto make_observer(OnNext n)
        -> typename std::enable_if<!is_observer<OnNext>::value,
                observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>>>::type {
        return  observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>>(
                            static_observer<T, OnNext, detail::OnErrorEmpty, detail::OnCompletedEmpty>(std::move(n), detail::OnErrorEmpty(), detail::OnCompletedEmpty()));
    }

    template<class T = void, class B = void>
    class observable;

namespace operators
{
    struct tag_operator {};
    template<class T>
    struct operator_base
    {
        typedef T value_type;
        typedef tag_operator operator_tag;
    };
    template<class T>
    class is_operator
    {
        template<class C>
        static typename C::operator_tag check(int);
        template<class C>
        static void check(...);
    public:
        static const bool value = std::is_convertible<decltype(check<T>(0)), tag_operator>::value;
    };

namespace detail {
    template<class Observer>
    class subscribe_to_observer_factory
    {
        Observer observer;
    public:
        subscribe_to_observer_factory(Observer o) : observer(std::move(o)) {}
        template<class Observable>
        auto operator()(Observable source)
            -> decltype(source.subscribe(std::move(observer))) {
            return      source.subscribe(std::move(observer));
        }
    };
    template<class OnNext, class OnError, class OnCompleted>
    class subscribe_factory
    {
        OnNext onnext;
        OnError onerror;
        OnCompleted oncompleted;
    public:
        subscribe_factory(OnNext n, OnError e, OnCompleted c)
            : onnext(std::move(n))
            , onerror(std::move(e))
            , oncompleted(std::move(c))
        {}
        template<class Observable>
        auto operator()(Observable source)
            -> decltype(source.subscribe(std::move(onnext), std::move(onerror), std::move(oncompleted))) {
            return      source.subscribe(std::move(onnext), std::move(onerror), std::move(oncompleted));
        }
    };
}
    template<class Observer>
    auto subscribe(Observer o)
        -> typename std::enable_if<is_observer<Observer>::value,
                detail::subscribe_to_observer_factory<Observer>>::type {
        return  detail::subscribe_to_observer_factory<Observer>(std::move(o));
    }

    template<class OnNext>
    auto subscribe(OnNext n)
        -> typename std::enable_if<!is_observer<OnNext>::value,
                detail::subscribe_factory<OnNext, rxcpp::detail::OnErrorEmpty, rxcpp::detail::OnCompletedEmpty>>::type {
        return  detail::subscribe_factory<OnNext, rxcpp::detail::OnErrorEmpty, rxcpp::detail::OnCompletedEmpty>(std::move(n), rxcpp::detail::OnErrorEmpty(), rxcpp::detail::OnCompletedEmpty());
    }

    template<class OnNext, class OnError>
    auto subscribe(OnNext n, OnError e)
        ->      detail::subscribe_factory<OnNext, OnError, rxcpp::detail::OnCompletedEmpty> {
        return  detail::subscribe_factory<OnNext, OnError, rxcpp::detail::OnCompletedEmpty>(std::move(n), std::move(e), rxcpp::detail::OnCompletedEmpty());
    }

    template<class OnNext, class OnError, class OnCompleted>
    auto subscribe(OnNext n, OnError e, OnCompleted c)
        ->      detail::subscribe_factory<OnNext, OnError, OnCompleted> {
        return  detail::subscribe_factory<OnNext, OnError, OnCompleted>(std::move(n), std::move(e), std::move(c));
    }

namespace detail {

    template<class T, class Observable, class Predicate>
    struct filter : public operator_base<T>
    {
        Observable source;
        Predicate test;

        template<class CT, class CP>
        static auto check(int) -> decltype((*(CP*)nullptr)(*(CT*)nullptr));
        template<class CT, class CP>
        static void check(...);
        filter(Observable o, Predicate p)
            : source(std::move(o))
            , test(std::move(p))
        {
            static_assert(std::is_convertible<decltype(check<T, Predicate>(0)), bool>::value, "filter Predicate must be a function with the signature bool(T)");
        }
        template<class I>
        void on_subscribe(observer<T, I> o) {
            o.add(source.subscribe(make_observer<T>(
            // on_next
                [this, o](T t) {
                    bool filtered = false;
                    try {
                       filtered = !this->test(t);
                    } catch(...) {
                        o.on_error(std::current_exception());
                        o.unsubscribe();
                    }
                    if (!filtered) {
                        o.on_next(std::move(t));
                    }
                },
            // on_error
                [o](std::exception_ptr e) {
                    o.on_error(e);
                },
            // on_completed
                [o]() {
                    o.on_completed();
                }
            )));
        }
    };
    template<class Predicate>
    class filter_factory
    {
        Predicate predicate;
    public:
        filter_factory(Predicate p) : predicate(std::move(p)) {}
        template<class Observable>
        auto operator()(Observable source)
            ->      observable<typename Observable::value_type, filter<typename Observable::value_type, Observable, Predicate>> {
            return  observable<typename Observable::value_type, filter<typename Observable::value_type, Observable, Predicate>>(
                                                                filter<typename Observable::value_type, Observable, Predicate>(source, std::move(predicate)));
        }
    };
}
    template<class Predicate>
    auto filter(Predicate p)
        ->      detail::filter_factory<Predicate> {
        return  detail::filter_factory<Predicate>(std::move(p));
    }

}
namespace rxo=operators;

namespace sources
{
    struct tag_source {};
    template<class T>
    struct source_base
    {
        typedef T value_type;
        typedef tag_source source_tag;
    };
    template<class T>
    class is_source
    {
        template<class C>
        static typename C::source_tag check(int);
        template<class C>
        static void check(...);
    public:
        static const bool value = std::is_convertible<decltype(check<T>(0)), tag_source>::value;
    };

namespace detail
{
    template<class T>
    struct range : public source_base<T>
    {
        struct state_type
        {
            T next;
            size_t remaining;
            ptrdiff_t step;
        };
        state_type init;
        range(T b, size_t c, ptrdiff_t s)
        {
            init.next = b;
            init.remaining = c;
            init.step = s;
        }
        template<class I>
        void on_subscribe(observer<T, I> o) {
            auto state = std::make_shared<state_type>(init);
            auto s = make_subscription();
            for (;state->remaining != 0; --state->remaining) {
                o.on_next(state->next);
                state->next = static_cast<T>(state->step + state->next);
            }
            o.on_completed();
        }
    };
}
}
namespace rxs=sources;

    template<class T, class SourceOperator>
    class observable
    {
        SourceOperator source_operator;
    public:
        typedef T value_type;

        static_assert(rxo::is_operator<SourceOperator>::value || rxs::is_source<SourceOperator>::value, "observable must wrap an operator or source");

        explicit observable(const SourceOperator& o)
            : source_operator(o)
        {}
        explicit observable(SourceOperator&& o)
            : source_operator(std::move(o))
        {}

        template<class I>
        auto subscribe(observer<T, I> o)
            -> decltype(make_subscription(o)) {
            source_operator.on_subscribe(o);
            return make_subscription(o);
        }

        template<class OnNext>
        auto subscribe(OnNext n)
            -> typename std::enable_if<!is_observer<OnNext>::value,
                decltype(make_subscription(make_observer<T>(std::move(n))))>::type {
            return subscribe(make_observer<T>(std::move(n)));
        }

        template<class OnNext, class OnError>
        auto subscribe(OnNext n, OnError e)
            -> decltype(make_subscription(make_observer<T>(std::move(n), std::move(e)))) {
            return subscribe(make_observer<T>(std::move(n), std::move(e)));
        }

        template<class OnNext, class OnError, class OnCompleted>
        auto subscribe(OnNext n, OnError e, OnCompleted c)
            -> decltype(make_subscription(make_observer<T>(std::move(n), std::move(e), std::move(c)))) {
            return subscribe(make_observer<T>(std::move(n), std::move(e), std::move(c)));
        }

        template<class Predicate>
        auto filter(Predicate p)
            ->      observable<T,   rxo::detail::filter<T, observable, Predicate>> {
            return  observable<T,   rxo::detail::filter<T, observable, Predicate>>(
                                    rxo::detail::filter<T, observable, Predicate>(*this, std::move(p)));
        }
    };

    // observable<> has static methods to construct observable sources and adaptors.
    // observable<> is not constructable
    template<>
    class observable<void, void>
    {
        ~observable();
    public:
        template<class T>
        static auto range(T start = 0, size_t count = std::numeric_limits<size_t>::max(), ptrdiff_t step = 1)
            ->      observable<T,   rxs::detail::range<T>> {
            return  observable<T,   rxs::detail::range<T>>(
                                    rxs::detail::range<T>(start, count, step));
        }
    };

    template<class T, class SourceOperator, class OperatorFactory>
    auto operator >> (observable<T, SourceOperator> source, OperatorFactory&& op)
        -> decltype(op(source)){
        return      op(source);
    }

}
#endif
