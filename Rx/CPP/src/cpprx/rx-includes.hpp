// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(CPPRX_RX_INCLUDES_HPP)
#define CPPRX_RX_INCLUDES_HPP

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

#include <exception>
#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <deque>
#include <thread>
#include <future>
#include <vector>
#include <queue>
#include <chrono>
#include <condition_variable>

// some configuration macros
#if defined(_MSC_VER)

#if _MSC_VER > 1600 
#define RXCPP_USE_RVALUEREF 1
#define RXCPP_USE_VARIADIC_TEMPLATES 0
#endif

#if _CPPRTTI
#define RXCPP_USE_RTTI 1
#endif

#elif defined(__clang__)

#if __has_feature(cxx_rvalue_references)
#define RXCPP_USE_RVALUEREF 1
#endif
#if __has_feature(cxx_rtti)
#define RXCPP_USE_RTTI 1
#endif
#if __has_feature(cxx_variadic_templates)
#define RXCPP_USE_VARIADIC_TEMPLATES 1
#endif

#endif


#include "rx-util.hpp"
#include "rx-base.hpp"
#include "rx-scheduler.hpp"
#include "rx-windows.hpp"
#include "rx-operators.hpp"

#pragma pop_macro("min")
#pragma pop_macro("max")

#endif
