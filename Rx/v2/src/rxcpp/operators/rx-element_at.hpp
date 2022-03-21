// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-element_at.hpp
  
    \brief Pulls an item located at a specified index location in the sequence of items and emits that item as its own sole emission.

    \param index  the index of the element to return.

    \return An observable that emit an item located at a specified index location.

    \sample
    \snippet element_at.cpp element_at sample
    \snippet output.txt element_at sample
*/

#if !defined(RXCPP_OPERATORS_RX_ELEMENT_AT_HPP)
#define RXCPP_OPERATORS_RX_ELEMENT_AT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct element_at_invalid_arguments {};

template<class... AN>
struct element_at_invalid : public rxo::operator_base<element_at_invalid_arguments<AN...>> {
    using type = observable<element_at_invalid_arguments<AN...>, element_at_invalid<AN...>>;
};
template<class... AN>
using element_at_invalid_t = typename element_at_invalid<AN...>::type;
    
template<class T>
struct element_at {
    using source_value_type = rxu::decay_t<T>;

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
        using this_type = element_at_observer<Subscriber>;
        using value_type = source_value_type;
        using dest_type = rxu::decay_t<Subscriber>;
        using observer_type = observer<value_type, this_type>;
        dest_type dest;
        mutable int current;

        element_at_observer(dest_type d, element_at_values v)
            : element_at_values(v),
              dest(d),
              current(0)
        {
        }
        template<typename U>
        void on_next(U&& v) const {
            if (current++ == this->index) {
                dest.on_next(std::forward<U>(v));
                dest.on_completed();
            }
        }
        void on_error(rxu::error_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            if(current <= this->index) {
                dest.on_error(rxu::make_error_ptr(std::range_error("index is out of bounds")));
            }
        }

        static subscriber<value_type, observer_type> make(dest_type d, element_at_values v) {
            return make_subscriber<value_type>(d, this_type(d, v));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(element_at_observer<Subscriber>::make(std::move(dest), initial)) {
        return      element_at_observer<Subscriber>::make(std::move(dest), initial);
    }
};

}

/*! @copydoc rx-element_at.hpp
*/
template<class... AN>
auto element_at(AN&&... an)
    ->      operator_factory<element_at_tag, AN...> {
     return operator_factory<element_at_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<element_at_tag>
{
    template<class Observable,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>
            >,
        class SourceValue = rxu::value_type_t<Observable>,
        class element_at = rxo::detail::element_at<SourceValue>>
    static auto member(Observable&& o, int index)
        -> decltype(o.template lift<SourceValue>(element_at(index))) {
        return      o.template lift<SourceValue>(element_at(index));
    }

    template<class... AN>
    static operators::detail::element_at_invalid_t<AN...> member(const AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "element_at takes (required int)");
    }
};
    
}

#endif
