// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_NOTIFICATION_HPP)
#define RXCPP_RX_NOTIFICATION_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace notifications {

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
    return lhs.Time() == rhs.Time() && lhs.Value() == rhs.Value();
}

template<class T>
std::ostream& operator<< (std::ostream& out, const recorded<T>& r) {
    out << "@" << r.Time() << "-" << r.Value();
    return out;
}

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
    typedef observer<T, dynamic_observer<T>> observer_type;
    typedef std::shared_ptr<notification_base<T>> type;

    virtual ~notification_base() {}

    virtual void out(std::ostream& out) const =0;
    virtual bool equals(const type& other) const = 0;
    virtual void accept(const observer_type& o) const =0;
};

}

template<typename T>
struct notification
{
    typedef typename detail::notification_base<T>::type type;
    typedef typename detail::notification_base<T>::observer_type observer_type;

private:
    typedef detail::notification_base<T> base;

    struct on_next_notification : public base {
        on_next_notification(T value) : value(std::move(value)) {
        }
        virtual void out(std::ostream& out) const {
            out << "on_next( " << value << ")";
        }
        virtual bool equals(const typename base::type& other) const {
            bool result = false;
            other->accept(make_observer_dynamic([this, &result](T value) {
                    result = this->value == value;
                }));
            return result;
        }
        virtual void accept(const typename base::observer_type& o) const {
            if (!o) {
                abort();
            }
            o->on_next(value);
        }
        const T value;
    };

    struct on_error_notification : public base {
        on_error_notification(std::exception_ptr ep) : ep(ep) {
        }
        virtual void out(std::ostream& out) const {
            out << "on_error(";
            try {
                std::rethrow_exception(ep);
            } catch (const std::exception& e) {
                out << e.what();
            } catch (...) {
                out << "<not derived from std::exception>";
            }
            out << ")";
        }
        virtual bool equals(const typename base::type& other) const {
            bool result = false;
            // not trying to compare exceptions
            other->accept(make_observer_dynamic(nullptr, [&result](std::exception_ptr){
                result = true;
            }));
            return result;
        }
        virtual void accept(const typename base::observer_type& o) const {
            if (!o) {
                abort();
            }
            o->on_error(ep);
        }
        const std::exception_ptr ep;
    };

    struct on_completed_notification : public base {
        on_completed_notification() {
        }
        virtual void out(std::ostream& out) const {
            out << "on_completed()";
        }
        virtual bool equals(const typename base::type& other) const {
            bool result = false;
            other->Accept(make_observer_dynamic(nullptr, nullptr, [&result](){
                result = true;
            }));
            return result;
        }
        virtual void accept(const typename base::observer_type& o) const {
            if (!o) {
                abort();
            }
            o->on_completed();
        }
    };

    struct exception_tag {};

    template<typename Exception>
    static
    type make_on_error(exception_tag&&, Exception&& e) {
        std::exception_ptr ep;
        try {
            throw std::forward<Exception>(e);
        }
        catch (...) {
            ep = std::current_exception();
        }
        return std::make_shared<on_error_notification>(ep);
    }

    struct exception_ptr_tag {};

    static
    type make_on_error(exception_ptr_tag&&, std::exception_ptr ep) {
        return std::make_shared<on_error_notification>(ep);
    }

public:
    static
    type make_on_next(T value) {
        return std::make_shared<on_next_notification>(std::move(value));
    }
    static
    type make_on_completed() {
        return std::make_shared<on_completed_notification>();
    }
    template<typename Exception>
    static
    type make_on_error(Exception&& e) {
        return make_on_error(typename std::conditional<
            std::is_same<typename std::decay<Exception>::type, std::exception_ptr>::value,
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
std::ostream& operator<< (std::ostream& out, const std::shared_ptr<detail::notification_base<T>>& n) {
    n->out(out);
    return out;
}

}

}

#endif
