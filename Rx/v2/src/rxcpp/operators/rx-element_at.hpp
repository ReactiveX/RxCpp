// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_ELEMENT_AT_HPP)
#define RXCPP_OPERATORS_RX_ELEMENT_AT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T>
struct element_at {
    typedef rxu::decay_t<T> source_value_type;

    struct element_at_values {
        element_at_values(int i)
                : index(i)
        {
        }
        int index;
    };

    element_at_values initial;
    
    element_at(int i)
        : initial(i)
    {
    }

    template<class Subscriber>
    struct element_at_observer : public element_at_values
    {
        typedef element_at_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;
        mutable int current;

        element_at_observer(dest_type d, element_at_values v)
            : element_at_values(v),
              dest(d),
              current(0)
        {
        }
        void on_next(source_value_type v) const {
            if (current++ == this->index) {
                dest.on_next(v);
                dest.on_completed();
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            if(current <= this->index) {
                dest.on_error(std::make_exception_ptr(std::range_error("index is out of bounds")));
            }
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d, element_at_values v) {
            return make_subscriber<value_type>(d, this_type(d, v));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(element_at_observer<Subscriber>::make(std::move(dest), initial)) {
        return      element_at_observer<Subscriber>::make(std::move(dest), initial);
    }
};

class element_at_factory
{
    int index;
public:
    element_at_factory(int i) : index(i) {}

    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(element_at<rxu::value_type_t<rxu::decay_t<Observable>>>(index))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(element_at<rxu::value_type_t<rxu::decay_t<Observable>>>(index));
    }
};

}


inline auto element_at(int index)
    ->      detail::element_at_factory {
    return  detail::element_at_factory(index);
}

}

}

#endif
