// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_FINALLY_HPP)
#define RXCPP_OPERATORS_RX_FINALLY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class LastCall>
struct finally
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<LastCall> last_call_type;
    last_call_type last_call;

    finally(last_call_type lc)
        : last_call(std::move(lc))
    {
    }

    template<class Subscriber>
    struct finally_observer
    {
        typedef finally_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;

        finally_observer(dest_type d)
            : dest(std::move(d))
        {
        }
        void on_next(source_value_type v) const {
            dest.on_next(v);
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d, const last_call_type& lc) {
            auto dl = d.get_subscription();
            composite_subscription cs;
            dl.add(cs);
            cs.add([=](){
                dl.unsubscribe();
                lc();
            });
            return make_subscriber<value_type>(cs, this_type(d));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(finally_observer<Subscriber>::make(std::move(dest), last_call)) {
        return      finally_observer<Subscriber>::make(std::move(dest), last_call);
    }
};

template<class LastCall>
class finally_factory
{
    typedef rxu::decay_t<LastCall> last_call_type;
    last_call_type last_call;
public:
    finally_factory(last_call_type lc) : last_call(std::move(lc)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(filter<rxu::value_type_t<rxu::decay_t<Observable>>, last_call_type>(last_call))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(filter<rxu::value_type_t<rxu::decay_t<Observable>>, last_call_type>(last_call));
    }
};

}

template<class LastCall>
auto finally(LastCall lc)
    ->      detail::finally_factory<LastCall> {
    return  detail::finally_factory<LastCall>(std::move(lc));
}

}

}

#endif
