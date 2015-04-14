// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_PAIRWISE_HPP)
#define RXCPP_OPERATORS_RX_PAIRWISE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T>
struct pairwise
{
    typedef rxu::decay_t<T> source_value_type;
    typedef std::tuple<source_value_type, source_value_type> value_type;

    template<class Subscriber>
    struct pairwise_observer
    {
        typedef pairwise_observer<Subscriber> this_type;
        typedef std::tuple<source_value_type, source_value_type> value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<T, this_type> observer_type;
        dest_type dest;
        mutable rxu::detail::maybe<source_value_type> remembered;

        pairwise_observer(dest_type d)
            : dest(std::move(d))
        {
        }
        void on_next(source_value_type v) const {
            if (remembered.empty()) {
                remembered.reset(v);
                return;
            }

            dest.on_next(std::make_tuple(remembered.get(), v));
            remembered.reset(v);
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<T, observer_type> make(dest_type d) {
            auto cs = d.get_subscription();
            return make_subscriber<T>(std::move(cs), observer_type(this_type(std::move(d))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(pairwise_observer<Subscriber>::make(std::move(dest))) {
        return      pairwise_observer<Subscriber>::make(std::move(dest));
    }
};

class pairwise_factory
{
public:
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<pairwise<rxu::value_type_t<rxu::decay_t<Observable>>>>>(pairwise<rxu::value_type_t<rxu::decay_t<Observable>>>())) {
        return      source.template lift<rxu::value_type_t<pairwise<rxu::value_type_t<rxu::decay_t<Observable>>>>>(pairwise<rxu::value_type_t<rxu::decay_t<Observable>>>());
    }
};

}

inline auto pairwise()
    ->      detail::pairwise_factory {
    return  detail::pairwise_factory();
}

}

}

#endif
