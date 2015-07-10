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
template<class T>
class is_operator
{
    template<class C>
    static typename C::operator_tag* check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), tag_operator*>::value;
};

}
namespace rxo=operators;

}

#include "operators/rx-amb.hpp"
#include "operators/rx-buffer_count.hpp"
#include "operators/rx-buffer_time.hpp"
#include "operators/rx-buffer_time_count.hpp"
#include "operators/rx-combine_latest.hpp"
#include "operators/rx-concat.hpp"
#include "operators/rx-concat_map.hpp"
#include "operators/rx-connect_forever.hpp"
#include "operators/rx-distinct_until_changed.hpp"
#include "operators/rx-filter.hpp"
#include "operators/rx-finally.hpp"
#include "operators/rx-flat_map.hpp"
#include "operators/rx-group_by.hpp"
#include "operators/rx-lift.hpp"
#include "operators/rx-map.hpp"
#include "operators/rx-merge.hpp"
#include "operators/rx-multicast.hpp"
#include "operators/rx-observe_on.hpp"
#include "operators/rx-on_error_resume_next.hpp"
#include "operators/rx-pairwise.hpp"
#include "operators/rx-publish.hpp"
#include "operators/rx-reduce.hpp"
#include "operators/rx-ref_count.hpp"
#include "operators/rx-repeat.hpp"
#include "operators/rx-replay.hpp"
#include "operators/rx-retry.hpp"
#include "operators/rx-scan.hpp"
#include "operators/rx-skip.hpp"
#include "operators/rx-skip_until.hpp"
#include "operators/rx-start_with.hpp"
#include "operators/rx-subscribe.hpp"
#include "operators/rx-subscribe_on.hpp"
#include "operators/rx-switch_on_next.hpp"
#include "operators/rx-take.hpp"
#include "operators/rx-take_until.hpp"
#include "operators/rx-tap.hpp"
#include "operators/rx-window.hpp"
#include "operators/rx-window_time.hpp"
#include "operators/rx-window_time_count.hpp"
#include "operators/rx-zip.hpp"
#endif
