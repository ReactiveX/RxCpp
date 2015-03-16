// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_DISTINCT_UNTIL_CHANGED_HPP)
#define RXCPP_OPERATORS_RX_DISTINCT_UNTIL_CHANGED_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T>
struct distinct_until_changed
{
    typedef rxu::decay_t<T> source_value_type;

    template<class Subscriber>
    struct distinct_until_changed_observer
    {
        typedef distinct_until_changed_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;
        mutable rxu::detail::maybe<source_value_type> remembered;

        distinct_until_changed_observer(dest_type d)
            : dest(d)
        {
        }
        void on_next(source_value_type v) const {
            if (remembered.empty() || v != remembered.get()) {
                remembered.reset(v);
                dest.on_next(v);
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d) {
            return make_subscriber<value_type>(d, this_type(d));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(distinct_until_changed_observer<Subscriber>::make(std::move(dest))) {
        return      distinct_until_changed_observer<Subscriber>::make(std::move(dest));
    }
};

class distinct_until_changed_factory
{
public:
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(distinct_until_changed<rxu::decay_t<Observable>>::value_type)) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(distinct_until_changed<rxu::decay_t<Observable>>::value_type);
    }
};

}

inline auto distinct_until_changed()
    ->      detail::distinct_until_changed_factory {
    return  detail::distinct_until_changed_factory();
}

}

}

#endif
