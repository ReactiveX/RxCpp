// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_PREDEF_HPP)
#define RXCPP_RX_PREDEF_HPP

#include "rx-includes.hpp"

namespace rxcpp {

template<class T>
class dynamic_observer;

template<class T, class I = dynamic_observer<T>>
class observer;

template<class T>
class dynamic_observable;

template<
    class T = void,
    class SourceObservable = typename std::conditional<std::is_same<T, void>::value,
        void, dynamic_observable<T>>::type>
class observable;

template<class T, class Source>
observable<T> make_dynamic_observable(Source&&);

struct tag_observable {};
template<class T>
struct observable_base {
    typedef tag_observable observable_tag;
    typedef T value_type;
};
template<class T>
class is_observable
{
    template<class C>
    static typename C::observable_tag check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<T>(0)), tag_observable>::value;
};

}

#endif
