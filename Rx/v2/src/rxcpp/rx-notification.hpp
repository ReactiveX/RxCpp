// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_NOTIFICATION_HPP)
#define RXCPP_RX_NOTIFICATION_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace notifications {

class subscription
{
    long s;
    long u;

public:
    explicit inline subscription(long s)
        : s(s), u(std::numeric_limits<long>::max()) {
    }
    inline subscription(long s, long u)
        : s(s), u(u) {
    }
    inline long subscribe() const {
        return s;
    }
    inline long unsubscribe() const {
        return u;
    }
};

inline bool operator == (subscription lhs, subscription rhs) {
    return lhs.subscribe() == rhs.subscribe() && lhs.unsubscribe() == rhs.unsubscribe();
}

inline std::ostream& operator<< (std::ostream& out, const subscription& s) {
    out << s.subscribe() << "-" << s.unsubscribe();
    return out;
}

namespace detail {

template<typename T>
struct notification_base
    : public std::enable_shared_from_this<notification_base<T>>
{
    typedef subscriber<T> observer_type;
    typedef std::shared_ptr<notification_base<T>> type;

    virtual ~notification_base() {}

    virtual void out(std::ostream& out) const =0;
    virtual bool equals(const type& other) const = 0;
    virtual void accept(const observer_type& o) const & =0;
    virtual void accept(const observer_type& o) && =0;
};

template<class T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v);

template<class T>
auto to_stream(std::ostream& os, const T& t, int, int)
    -> decltype(os << t) {
    return      os << t;
}

#if RXCPP_USE_RTTI
template<class T>
std::ostream& to_stream(std::ostream& os, const T&, int, ...) {
    return os << "< " << typeid(T).name() << " does not support ostream>";
}
#endif

template<class T>
std::ostream& to_stream(std::ostream& os, const T&, ...) {
    return os << "<the value does not support ostream>";
}

template<class T>
inline std::ostream& ostreamvector (std::ostream& os, const std::vector<T>& v) {
    os << "[";
    bool doemit = false;
    for(auto& i : v) {
        if (doemit) {
            os << ", ";
        } else {
            doemit = true;
        }
        to_stream(os, i, 0, 0);
    }
    os << "]";
    return os;
}

template<class T>
inline std::ostream& operator<< (std::ostream& os, const std::vector<T>& v) {
    return ostreamvector(os, v);
}

template<class T>
auto equals(const T& lhs, const T& rhs, int)
    -> decltype(bool(lhs == rhs)) {
    return lhs == rhs;
}

template<class T>
bool equals(const T&, const T&, ...) {
    rxu::throw_exception(std::runtime_error("value does not support equality tests"));
    return false;
}

}

template<typename T>
struct notification
{
    typedef typename detail::notification_base<T>::type type;
    typedef typename detail::notification_base<T>::observer_type observer_type;

private:
    typedef detail::notification_base<T> base;

    struct on_next_notification : public base {
        on_next_notification(T&& value) : value(std::move(value)) {}
        on_next_notification(const T& value) : value(value) {}
        on_next_notification(const on_next_notification& o) : value(o.value) {}
        on_next_notification(const on_next_notification&& o) : value(std::move(o.value)) {}
        on_next_notification& operator=(on_next_notification o) { value = std::move(o.value); return *this; }
        void out(std::ostream& os) const override {
            os << "on_next( ";
            detail::to_stream(os, value, 0, 0);
            os << ")";
        }
        bool equals(const typename base::type& other) const override {
            bool result = false;
            other->accept(make_subscriber<T>(make_observer_dynamic<T>([this, &result](T v) {
                    result = detail::equals(this->value, v, 0);
                })));
            return result;
        }
        void accept(const typename base::observer_type& o) const & override{
            o.on_next(value);
        }

        void accept(const typename base::observer_type& o) && override {
            o.on_next(std::move(value));
        }
        T value;
    };

    struct on_error_notification : public base {
        on_error_notification(rxu::error_ptr ep) : ep(ep) {
        }
        on_error_notification(const on_error_notification& o) : ep(o.ep) {}
        on_error_notification(const on_error_notification&& o) : ep(std::move(o.ep)) {}
        on_error_notification& operator=(on_error_notification o) { ep = std::move(o.ep); return *this; }
        void out(std::ostream& os) const override {
            os << "on_error(";
            os << rxu::what(ep);
            os << ")";
        }
        bool equals(const typename base::type& other) const override {
            bool result = false;
            // not trying to compare exceptions
            other->accept(make_subscriber<T>(make_observer_dynamic<T>([](T){}, [&result](rxu::error_ptr){
                result = true;
            })));
            return result;
        }
        void accept(const typename base::observer_type& o) const & override{
            o.on_error(ep);
        }

        void accept(const typename base::observer_type& o) && override{
            o.on_error(ep);
        }
        const rxu::error_ptr ep;
    };

    struct on_completed_notification : public base {
        on_completed_notification() {
        }
        void out(std::ostream& os) const override {
            os << "on_completed()";
        }
        bool equals(const typename base::type& other) const override {
            bool result = false;
            other->accept(make_subscriber<T>(make_observer_dynamic<T>([](T){}, [&result](){
                result = true;
            })));
            return result;
        }
        void accept(const typename base::observer_type& o) const & override{
            o.on_completed();
        }

        void accept(const typename base::observer_type& o) && override{
            o.on_completed();
        }
    };

    struct exception_tag {};

    template<typename Exception>
    static
    type make_on_error(exception_tag&&, Exception&& e) {
        rxu::error_ptr ep = rxu::make_error_ptr(std::forward<Exception>(e));
        return std::make_shared<on_error_notification>(ep);
    }

    struct exception_ptr_tag {};

    static
    type make_on_error(exception_ptr_tag&&, rxu::error_ptr ep) {
        return std::make_shared<on_error_notification>(ep);
    }

public:
    template<typename U>
    static type on_next(U&& value) {
        return std::make_shared<on_next_notification>(std::forward<U>(value));
    }

    static type on_completed() {
        return std::make_shared<on_completed_notification>();
    }

    template<typename Exception>
    static type on_error(Exception&& e) {
        return make_on_error(typename std::conditional<
            std::is_same<rxu::decay_t<Exception>, rxu::error_ptr>::value,
                exception_ptr_tag, exception_tag>::type(),
            std::forward<Exception>(e));
    }
};

template<class T>
bool operator == (const std::shared_ptr<detail::notification_base<T>>& lhs, const std::shared_ptr<detail::notification_base<T>>& rhs) {
    if (!lhs && !rhs) {return true;}
    if (!lhs || !rhs) {return false;}
    return lhs->equals(rhs);
}

template<class T>
std::ostream& operator<< (std::ostream& os, const std::shared_ptr<detail::notification_base<T>>& n) {
    n->out(os);
    return os;
}


template<class T>
class recorded
{
    long t;
    T v;
public:
    recorded(long t, T v)
        : t(t), v(v) {
    }
    long time() const {
        return t;
    }
    const T& value() const {
        return v;
    }
};

template<class T>
bool operator == (recorded<T> lhs, recorded<T> rhs) {
    return lhs.time() == rhs.time() && lhs.value() == rhs.value();
}

template<class T>
std::ostream& operator<< (std::ostream& out, const recorded<T>& r) {
    out << "@" << r.time() << "-" << r.value();
    return out;
}
 
}
namespace rxn=notifications;

inline std::ostream& operator<< (std::ostream& out, const std::vector<rxcpp::notifications::subscription>& vs) {
    return rxcpp::notifications::detail::ostreamvector(out, vs);
}
template<class T>
inline std::ostream& operator<< (std::ostream& out, const std::vector<rxcpp::notifications::recorded<T>>& vr) {
    return rxcpp::notifications::detail::ostreamvector(out, vr);
}

}

#endif
