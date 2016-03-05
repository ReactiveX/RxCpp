// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_IGNORE_ELEMENTS_HPP)
#define RXCPP_OPERATORS_RX_IGNORE_ELEMENTS_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T>
struct ignore_elements {
    typedef rxu::decay_t<T> source_value_type;

    template<class Subscriber>
    struct ignore_elements_observer
    {
        typedef ignore_elements_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;

        ignore_elements_observer(dest_type d)
            : dest(d)
        {
        }

        void on_next(source_value_type) const {
            // no-op; ignore element
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
        -> decltype(ignore_elements_observer<Subscriber>::make(std::move(dest))) {
        return      ignore_elements_observer<Subscriber>::make(std::move(dest));
    }
};

class ignore_elements_factory
{
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(ignore_elements<rxu::value_type_t<rxu::decay_t<Observable>>>())) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(ignore_elements<rxu::value_type_t<rxu::decay_t<Observable>>>());
    }
};

}


inline auto ignore_elements()
    ->      detail::ignore_elements_factory {
    return  detail::ignore_elements_factory();
}

}

}

#endif
