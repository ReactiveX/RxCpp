// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPRX_RX_UTIL_HPP)
#define CPPRX_RX_UTIL_HPP
#pragma once

namespace rxcpp { namespace util {

    template<class Type>
    struct identity 
    {
        typedef Type type;
        Type operator()(const Type& left) const {return left;}
    };

}}

#endif