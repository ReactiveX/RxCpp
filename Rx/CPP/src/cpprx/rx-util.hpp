// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPRX_RX_UTIL_HPP)
#define CPPRX_RX_UTIL_HPP
#pragma once

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