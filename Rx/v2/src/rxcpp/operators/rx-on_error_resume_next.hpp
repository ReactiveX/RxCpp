// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_ON_ERROR_RESUME_NEXT_HPP)
#define RXCPP_OPERATORS_RX_ON_ERROR_RESUME_NEXT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {


template<class T, class Selector>
struct on_error_resume_next
{
    typedef rxu::decay_t<T> value_type;
    typedef rxu::decay_t<Selector> select_type;
    typedef decltype((*(select_type*)nullptr)(std::exception_ptr())) fallback_type;
    select_type selector;

    on_error_resume_next(select_type s)
        : selector(std::move(s))
    {
    }

    template<class Subscriber>
    struct on_error_resume_next_observer
    {
        typedef on_error_resume_next_observer<Subscriber> this_type;
        typedef rxu::decay_t<T> value_type;
        typedef rxu::decay_t<Selector> select_type;
        typedef decltype((*(select_type*)nullptr)(std::exception_ptr())) fallback_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<T, this_type> observer_type;
        dest_type dest;
        composite_subscription lifetime;
        select_type selector;

        on_error_resume_next_observer(dest_type d, composite_subscription cs, select_type s)
            : dest(std::move(d))
            , lifetime(std::move(cs))
            , selector(std::move(s))
        {
            dest.add(lifetime);
        }
        void on_next(value_type v) const {
            dest.on_next(std::move(v));
        }
        void on_error(std::exception_ptr e) const {
            auto selected = on_exception(
                [&](){
                    return this->selector(std::move(e));},
                dest);
            if (selected.empty()) {
                return;
            }
            selected->subscribe(dest);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<T, observer_type> make(dest_type d, select_type s) {
            auto cs = composite_subscription();
            return make_subscriber<T>(cs, observer_type(this_type(std::move(d), cs, std::move(s))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(on_error_resume_next_observer<Subscriber>::make(std::move(dest), selector)) {
        return      on_error_resume_next_observer<Subscriber>::make(std::move(dest), selector);
    }
};

template<class Selector>
class on_error_resume_next_factory
{
    typedef rxu::decay_t<Selector> select_type;
    select_type selector;
public:
    on_error_resume_next_factory(select_type s) : selector(std::move(s)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<on_error_resume_next<rxu::value_type_t<rxu::decay_t<Observable>>, select_type>>>(on_error_resume_next<rxu::value_type_t<rxu::decay_t<Observable>>, select_type>(selector))) {
        return      source.template lift<rxu::value_type_t<on_error_resume_next<rxu::value_type_t<rxu::decay_t<Observable>>, select_type>>>(on_error_resume_next<rxu::value_type_t<rxu::decay_t<Observable>>, select_type>(selector));
    }
};

}

template<class Selector>
auto on_error_resume_next(Selector&& p)
    ->      detail::on_error_resume_next_factory<Selector> {
    return  detail::on_error_resume_next_factory<Selector>(std::forward<Selector>(p));
}

}

}

#endif
