// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_INCLUDES_HPP)
#define RXCPP_RX_INCLUDES_HPP

#include "rx-trace.hpp"

// some configuration macros
#if defined(_MSC_VER)

#if _MSC_VER > 1600
#pragma warning(disable: 4348) // false positives on : redefinition of default parameter : parameter 2
#define RXCPP_USE_RVALUEREF 1
#endif

#if _MSC_VER >= 1800
#define RXCPP_USE_VARIADIC_TEMPLATES 1
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

#elif defined(__GNUG__)

#define GCC_VERSION (__GNUC__ * 10000 + \
                     __GNUC_MINOR__ * 100 + \
                     __GNUC_PATCHLEVEL__)

#if GCC_VERSION >= 40801
#define RXCPP_USE_RVALUEREF 1
#endif

#if GCC_VERSION >= 40400
#define RXCPP_USE_VARIADIC_TEMPLATES 1
#endif

#if defined(__GXX_RTTI)
#define RXCPP_USE_RTTI 1
#endif

#endif

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#define RXCPP_USE_WINRT 0
#else
#define RXCPP_USE_WINRT 1
#endif

#if defined(RXCPP_FORCE_USE_VARIADIC_TEMPLATES)
#undef RXCPP_USE_VARIADIC_TEMPLATES
#define RXCPP_USE_VARIADIC_TEMPLATES RXCPP_FORCE_USE_VARIADIC_TEMPLATES
#endif

#if defined(RXCPP_FORCE_USE_RVALUEREF)
#undef RXCPP_USE_RVALUEREF
#define RXCPP_USE_RVALUEREF RXCPP_FORCE_USE_RVALUEREF
#endif

#if defined(RXCPP_FORCE_USE_RTTI)
#undef RXCPP_USE_RTTI
#define RXCPP_USE_RTTI RXCPP_FORCE_USE_RTTI
#endif

#if defined(RXCPP_FORCE_USE_WINRT)
#undef RXCPP_USE_WINRT
#define RXCPP_USE_WINRT RXCPP_FORCE_USE_WINRT
#endif

#if defined(_MSC_VER) && !RXCPP_USE_VARIADIC_TEMPLATES
// resolve args needs enough to store all the possible resolved args
#define _VARIADIC_MAX 10
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#if (TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)
#define RXCPP_ON_IOS
#endif
#endif

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

#include <stdlib.h>

#include <cstddef>

#include <iostream>
#include <iomanip>

#include <exception>
#include <functional>
#include <memory>
#include <array>
#include <vector>
#include <algorithm>
#include <atomic>
#include <map>
#include <set>
#include <mutex>
#include <deque>
#include <thread>
#include <future>
#include <vector>
#include <list>
#include <queue>
#include <chrono>
#include <condition_variable>
#include <initializer_list>
#include <typeinfo>
#include <tuple>

#if defined(RXCPP_ON_IOS)
#include <pthread.h>
#endif

#include "rx-util.hpp"
#include "rx-predef.hpp"
#include "rx-subscription.hpp"
#include "rx-observer.hpp"
#include "rx-scheduler.hpp"
#include "rx-subscriber.hpp"
#include "rx-notification.hpp"
#include "rx-coordination.hpp"
#include "rx-sources.hpp"
#include "rx-subjects.hpp"
#include "rx-operators.hpp"
#include "rx-observable.hpp"
#include "rx-connectable_observable.hpp"
#include "rx-grouped_observable.hpp"

#pragma pop_macro("min")
#pragma pop_macro("max")

#endif
