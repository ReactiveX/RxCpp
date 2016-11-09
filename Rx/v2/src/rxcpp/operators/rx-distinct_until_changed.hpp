// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-distinct_until_changed.hpp

    \brief For each item from this observable, filter out consequentially repeated values and emit only changes from the new observable that is returned.

    \return  Observable that emits those items from the source observable that are distinct from their immediate predecessors.

    \sample
    \snippet distinct_until_changed.cpp distinct_until_changed sample
    \snippet output.txt distinct_until_changed sample
*/

#if !defined(RXCPP_OPERATORS_RX_DISTINCT_UNTIL_CHANGED_HPP)
#define RXCPP_OPERATORS_RX_DISTINCT_UNTIL_CHANGED_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct distinct_until_changed_invalid_arguments {};

template<class... AN>
struct distinct_until_changed_invalid : public rxo::operator_base<distinct_until_changed_invalid_arguments<AN...>> {
    using type = observable<distinct_until_changed_invalid_arguments<AN...>, distinct_until_changed_invalid<AN...>>;
};
template<class... AN>
using distinct_until_changed_invalid_t = typename distinct_until_changed_invalid<AN...>::type;

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

        static subscriber<value_type, observer_type> make(dest_type d) {
            return make_subscriber<value_type>(d, this_type(d));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(distinct_until_changed_observer<Subscriber>::make(std::move(dest))) {
        return      distinct_until_changed_observer<Subscriber>::make(std::move(dest));
    }
};

}

/*! @copydoc rx-distinct_until_changed.hpp
*/
template<class... AN>
auto distinct_until_changed(AN&&... an)
    ->      operator_factory<distinct_until_changed_tag, AN...> {
     return operator_factory<distinct_until_changed_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<distinct_until_changed_tag>
{
    template<class Observable,
            class SourceValue = rxu::value_type_t<Observable>,
            class Enabled = rxu::enable_if_all_true_type_t<
                is_observable<Observable>,
                is_hashable<SourceValue>>,
            class DistinctUntilChanged = rxo::detail::distinct_until_changed<SourceValue>>
    static auto member(Observable&& o)
    -> decltype(o.template lift<SourceValue>(DistinctUntilChanged())) {
        return  o.template lift<SourceValue>(DistinctUntilChanged());
    }

    template<class... AN>
    static operators::detail::distinct_until_changed_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "distinct_until_changed takes no arguments");
    }
};

}

#endif
