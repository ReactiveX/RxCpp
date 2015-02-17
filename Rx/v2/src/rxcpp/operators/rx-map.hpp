// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_MAP_HPP)
#define RXCPP_OPERATORS_RX_MAP_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {


template<class T, class Selector>
struct map
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Selector> select_type;
    typedef decltype((*(select_type*)nullptr)(*(source_value_type*)nullptr)) value_type;
    select_type selector;

    map(select_type s)
        : selector(std::move(s))
    {
    }

    template<class Subscriber>
    struct map_observer
    {
        typedef map_observer<Subscriber> this_type;
        typedef decltype((*(select_type*)nullptr)(*(source_value_type*)nullptr)) value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<T, this_type> observer_type;
        dest_type dest;
        select_type selector;

        map_observer(dest_type d, select_type s)
            : dest(std::move(d))
            , selector(std::move(s))
        {
        }
        void on_next(source_value_type v) const {
            auto selected = on_exception(
                [&](){
                    return this->selector(std::move(v));},
                dest);
            if (selected.empty()) {
                return;
            }
            dest.on_next(std::move(selected.get()));
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<T, observer_type> make(dest_type d, select_type s) {
            auto cs = d.get_subscription();
            return make_subscriber<T>(std::move(cs), observer_type(this_type(std::move(d), std::move(s))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(map_observer<Subscriber>::make(std::move(dest), selector)) {
        return      map_observer<Subscriber>::make(std::move(dest), selector);
    }
};

template<class Selector>
class map_factory
{
    typedef rxu::decay_t<Selector> select_type;
    select_type selector;
public:
    map_factory(select_type s) : selector(std::move(s)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<map<rxu::value_type_t<rxu::decay_t<Observable>>, select_type>>>(map<rxu::value_type_t<rxu::decay_t<Observable>>, select_type>(selector))) {
        return      source.template lift<rxu::value_type_t<map<rxu::value_type_t<rxu::decay_t<Observable>>, select_type>>>(map<rxu::value_type_t<rxu::decay_t<Observable>>, select_type>(selector));
    }
};

}

template<class Selector>
auto map(Selector&& p)
    ->      detail::map_factory<Selector> {
    return  detail::map_factory<Selector>(std::forward<Selector>(p));
}

}

}

#endif
