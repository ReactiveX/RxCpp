// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_COMPOSITE_EXCEPTION_HPP)
#define RXCPP_SOURCES_RX_COMPOSITE_EXCEPTION_HPP

#include "rx-includes.hpp"

namespace rxcpp {

    struct composite_exception : std::exception {

        typedef std::vector<std::exception_ptr> exception_values;

        virtual const char *what() const noexcept {
            return "rxcpp composite exception";
        }

        virtual bool empty() const {
            return exceptions.empty();
        }

        virtual void add(std::exception_ptr exception_ptr) {
            exceptions.push_back(exception_ptr);
        }

        exception_values exceptions;
    };
}


#endif
