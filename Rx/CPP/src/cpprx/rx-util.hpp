// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "rx-includes.hpp"

#if !defined(CPPRX_RX_UTIL_HPP)
#define CPPRX_RX_UTIL_HPP

#if !defined(RXCPP_THREAD_LOCAL)
#if defined(_MSC_VER)
#define RXCPP_THREAD_LOCAL __declspec(thread)
#else
#define RXCPP_THREAD_LOCAL __thread
#endif
#endif

#if !defined(RXCPP_SELECT_ANY)
#if defined(_MSC_VER)
#define RXCPP_SELECT_ANY __declspec(selectany)
#else
#define RXCPP_SELECT_ANY 
#endif
#endif

#define RXCPP_CONCAT(Prefix, Suffix) Prefix ## Suffix
#define RXCPP_CONCAT_EVALUATE(Prefix, Suffix) RXCPP_CONCAT(Prefix, Suffix)

#define RXCPP_MAKE_IDENTIFIER(Prefix) RXCPP_CONCAT_EVALUATE(Prefix, __LINE__)

namespace rxcpp { namespace util {

    template<class Type>
    struct identity 
    {
        typedef Type type;
        Type operator()(const Type& left) const {return left;}
    };

    template <class T>
    class maybe
    {
        bool is_set;
        typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type 
            storage;
    public:
        maybe()
        : is_set(false)
        {
        }

        maybe(T value)
        : is_set(false)
        {
            new (reinterpret_cast<T*>(&storage)) T(value);
            is_set = true;
        }

        maybe(const maybe& other)
        : is_set(false)
        {
            if (other.is_set) {
                new (reinterpret_cast<T*>(&storage)) T(*other.get());
                is_set = true;
            }
        }
        maybe(maybe&& other)
        : is_set(false)
        {
            if (other.is_set) {
                new (reinterpret_cast<T*>(&storage)) T(std::move(*other.get()));
                is_set = true;
                other.reset();
            }
        }

        ~maybe()
        {
            reset();
        }

        void reset()
        {
            if (is_set) {
                is_set = false;
                reinterpret_cast<T*>(&storage)->~T();
            }
        }

        T* get() {
            return is_set ? reinterpret_cast<T*>(&storage) : 0;
        }

        const T* get() const {
            return is_set ? reinterpret_cast<const T*>(&storage) : 0;
        }

        void set(const T& value) {
            if (is_set) {
                *reinterpret_cast<T*>(&storage) = value;
            } else {
                new (reinterpret_cast<T*>(&storage)) T(value);
                is_set = true;
            }
        }
        void set(T&& value) {
            if (is_set) {
                *reinterpret_cast<T*>(&storage) = std::move(value);
            } else {
                new (reinterpret_cast<T*>(&storage)) T(std::move(value));
                is_set = true;
            }
        }

        T& operator*() { return *get(); }
        const T& operator*() const { return *get(); }
        T* operator->() { return get(); }
        const T* operator->() const { return get(); }

        maybe& operator=(const T& other) {
            set(other);
        }
        maybe& operator=(const maybe& other) {
            if (const T* pother = other.get()) {
                set(*pother);
            } else {
                reset();
            }
            return *this;
        }

        // boolean-like operators
        operator T*() { return get(); }
        operator const T*() const { return get(); }

    private:
        
    };

    template<class T>
    struct reveal_type {private: reveal_type();};

#if RXCPP_USE_VARIADIC_TEMPLATES
    template <int... Indices> 
    struct tuple_indices;
    template <> 
    struct tuple_indices<-1> {                // for an empty std::tuple<> there is no entry
        typedef tuple_indices<> type;
    };
    template <int... Indices>
    struct tuple_indices<0, Indices...> {     // stop the recursion when 0 is reached
        typedef tuple_indices<0, Indices...> type;
    };
    template <int Index, int... Indices>
    struct tuple_indices<Index, Indices...> { // recursively build a sequence of indices
        typedef typename tuple_indices<Index - 1, Index, Indices...>::type type;
    };

    template <typename T>
    struct make_tuple_indices {
        typedef typename tuple_indices<std::tuple_size<T>::value - 1>::type type;
    };

    namespace detail {
    template<class T>
    struct tuple_dispatch;
    template<size_t... DisptachIndices>
    struct tuple_dispatch<tuple_indices<DisptachIndices...>> {
        template<class F, class T>
        static
        auto call(F&& f, T&& t) 
            -> decltype (std::forward<F>(f)(std::get<DisptachIndices>(std::forward<T>(t))...)) {
            return       std::forward<F>(f)(std::get<DisptachIndices>(std::forward<T>(t))...);
        }
    };}

    template<class F, class T>
    auto tuple_dispatch(F&& f, T&& t) 
        -> decltype(detail::tuple_dispatch<typename make_tuple_indices<typename std::decay<T>::type>::type>::call(std::forward<F>(f), std::forward<T>(t))) {
        return      detail::tuple_dispatch<typename make_tuple_indices<typename std::decay<T>::type>::type>::call(std::forward<F>(f), std::forward<T>(t));
    }

    struct as_tuple {
        template<class... AsTupleNext>
        auto operator()(AsTupleNext... x) 
            -> decltype(std::make_tuple(std::move(x)...)) const {
            return      std::make_tuple(std::move(x)...);}
    };
#endif //RXCPP_USE_VARIADIC_TEMPLATES

    struct pass_through {
        template<class X>
        X operator()(X x) const {return std::move(x);}
    };

    template<typename Function>
	class unwinder
	{
	public:
		~unwinder()
		{
			if (!!function)
			{
				try {
					(*function)();
				} catch (...) {
					std::unexpected();
				}
			}
		}

		explicit unwinder(Function* functionArg)
			: function(functionArg)
		{
		}

		void dismiss()
		{
			function = nullptr;
		}

	private:
		unwinder();
		unwinder(const unwinder&);
		unwinder& operator=(const unwinder&);

		Function* function;
	};

}}

#define RXCPP_UNWIND(Name, Function) \
	RXCPP_UNWIND_EXPLICIT(uwfunc_ ## Name, Name, Function)

#define RXCPP_UNWIND_AUTO(Function) \
	RXCPP_UNWIND_EXPLICIT(RXCPP_MAKE_IDENTIFIER(uwfunc_), RXCPP_MAKE_IDENTIFIER(unwind_), Function)

#define RXCPP_UNWIND_EXPLICIT(FunctionName, UnwinderName, Function) \
	auto FunctionName = (Function); \
	rxcpp::util::unwinder<decltype(FunctionName)> UnwinderName(std::addressof(FunctionName))

#endif