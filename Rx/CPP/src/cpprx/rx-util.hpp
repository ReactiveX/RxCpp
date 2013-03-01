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