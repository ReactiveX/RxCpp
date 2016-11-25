// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OPERATORS_HPP)
#define RXCPP_RX_OPERATORS_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace operators {

struct tag_operator {};
template<class T>
struct operator_base
{
    typedef T value_type;
    typedef tag_operator operator_tag;
};

namespace detail {

template<class T, class =rxu::types_checked>
struct is_operator : std::false_type
{
};

template<class T>
struct is_operator<T, rxu::types_checked_t<typename T::operator_tag>> 
    : std::is_convertible<typename T::operator_tag*, tag_operator*>
{
};

}

template<class T, class Decayed = rxu::decay_t<T>>
struct is_operator : detail::is_operator<Decayed>
{
};


}
namespace rxo=operators;

template<class Tag> 
struct member_overload
{
    template<class... AN>
    static auto member(AN&&...) ->
                typename Tag::template include_header<std::false_type> {
        return  typename Tag::template include_header<std::false_type>();
    }
};

template<class T, class... AN>
struct delayed_type{using value_type = T; static T value(AN**...) {return T{};}};

template<class T, class... AN>
using delayed_type_t = rxu::value_type_t<delayed_type<T, AN...>>;

template<class Tag, class... AN, class Overload = member_overload<rxu::decay_t<Tag>>>
auto observable_member(Tag, AN&&... an) -> 
    decltype(Overload::member(std::forward<AN>(an)...)) {
    return   Overload::member(std::forward<AN>(an)...);
}

template<class Tag, class... AN>
class operator_factory
{
    using this_type = operator_factory<Tag, AN...>;
    using tag_type = rxu::decay_t<Tag>;
    using tuple_type = std::tuple<rxu::decay_t<AN>...>;
    
    tuple_type an;

public:
    operator_factory(tuple_type an)
        : an(std::move(an))
    {
    }

    template<class... ZN>
    auto operator()(tag_type t, ZN&&... zn) const
        -> decltype(observable_member(t, std::forward<ZN>(zn)...)) {
        return      observable_member(t, std::forward<ZN>(zn)...);
    }

    template<class Observable>
    auto operator()(Observable source) const 
        -> decltype(rxu::apply(std::tuple_cat(std::make_tuple(tag_type{}, source), (*(tuple_type*)nullptr)), (*(this_type*)nullptr))) {
        return      rxu::apply(std::tuple_cat(std::make_tuple(tag_type{}, source),                      an),                  *this);
    }
};

}

#include "operators/rx-amb.hpp"
#include "operators/rx-buffer_count.hpp"
#include "operators/rx-buffer_time.hpp"
#include "operators/rx-buffer_time_count.hpp"
#include "operators/rx-concat.hpp"
#include "operators/rx-concat_map.hpp"
#include "operators/rx-connect_forever.hpp"
#include "operators/rx-finally.hpp"
#include "operators/rx-flat_map.hpp"
#include "operators/rx-lift.hpp"
#include "operators/rx-map.hpp"
#include "operators/rx-merge.hpp"
#include "operators/rx-multicast.hpp"
#include "operators/rx-observe_on.hpp"
#include "operators/rx-on_error_resume_next.hpp"
#include "operators/rx-pairwise.hpp"
#include "operators/rx-publish.hpp"
#include "operators/rx-ref_count.hpp"
#include "operators/rx-repeat.hpp"
#include "operators/rx-replay.hpp"
#include "operators/rx-retry.hpp"
#include "operators/rx-sample_time.hpp"
#include "operators/rx-scan.hpp"
#include "operators/rx-sequence_equal.hpp"
#include "operators/rx-skip.hpp"
#include "operators/rx-skip_last.hpp"
#include "operators/rx-skip_until.hpp"
#include "operators/rx-start_with.hpp"
#include "operators/rx-subscribe.hpp"
#include "operators/rx-subscribe_on.hpp"
#include "operators/rx-switch_if_empty.hpp"
#include "operators/rx-switch_on_next.hpp"
#include "operators/rx-take.hpp"
#include "operators/rx-take_last.hpp"
#include "operators/rx-take_until.hpp"
#include "operators/rx-tap.hpp"
#include "operators/rx-time_interval.hpp"
#include "operators/rx-timeout.hpp"
#include "operators/rx-timestamp.hpp"
#include "operators/rx-window.hpp"
#include "operators/rx-window_time.hpp"
#include "operators/rx-window_time_count.hpp"
#include "operators/rx-window_toggle.hpp"

namespace rxcpp {

struct all_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-all.hpp>");
    };
};
    
struct any_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-any.hpp>");
    };
};

struct exists_tag : any_tag {};
struct contains_tag : any_tag {};
    
struct combine_latest_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-combine_latest.hpp>");
    };
};

struct debounce_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-debounce.hpp>");
    };
};

struct delay_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-delay.hpp>");
    };
};

struct distinct_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-distinct.hpp>");
    };
};

struct distinct_until_changed_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-distinct_until_changed.hpp>");
    };
};

struct element_at_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-element_at.hpp>");
    };
};

struct filter_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-filter.hpp>");
    };
};

struct group_by_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-group_by.hpp>");
    };
};

struct ignore_elements_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-ignore_elements.hpp>");
    };
};

class empty_error: public std::runtime_error
{
    public:
        explicit empty_error(const std::string& msg):
            std::runtime_error(msg)
        {}
};
struct reduce_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-reduce.hpp>");
    };
};
struct first_tag  {};
struct last_tag : reduce_tag {};
struct sum_tag : reduce_tag {};
struct average_tag : reduce_tag {};
struct min_tag : reduce_tag {};
struct max_tag : reduce_tag {};

struct with_latest_from_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-with_latest_from.hpp>");
    };
};

struct zip_tag {
    template<class Included>
    struct include_header{
        static_assert(Included::value, "missing include: please #include <rxcpp/operators/rx-zip.hpp>");
    };
};

}

#endif
