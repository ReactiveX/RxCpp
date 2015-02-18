// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_START_WITH_HPP)
#define RXCPP_OPERATORS_RX_START_WITH_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

template<class Observable, class Value0, class... ValueN>
auto start_with(Observable o, Value0 v0, ValueN... vn)
    -> decltype(rxs::from(rxu::value_type_t<Observable>(v0), rxu::value_type_t<Observable>(vn)...).concat(o)) {
    return      rxs::from(rxu::value_type_t<Observable>(v0), rxu::value_type_t<Observable>(vn)...).concat(o);
}

}

}

#endif
