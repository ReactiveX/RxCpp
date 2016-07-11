// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_EMPTY_HPP)
#define RXCPP_SOURCES_RX_EMPTY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

template<class T>
auto empty()
    -> decltype(from<T>()) {
    return      from<T>();
}
template<class T, class Coordination>
auto empty(Coordination cn)
    -> decltype(from<T>(std::move(cn))) {
    return      from<T>(std::move(cn));
}

}

}

#endif
