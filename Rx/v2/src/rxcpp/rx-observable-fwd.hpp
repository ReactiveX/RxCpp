// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_OBSERVABLE_FWD_HPP)
#define RXCPP_RX_OBSERVABLE_FWD_HPP

#include <type_traits>

namespace rxcpp {

template<class T>
class dynamic_observable;

template<
    class T = void,
    class SourceObservable = typename std::conditional_t<rxcpp::is_same_v<T, void>, void, dynamic_observable<T>>>
class observable;

template<class T, class Source>
observable<T> make_observable_dynamic(Source&&);

}

#endif
