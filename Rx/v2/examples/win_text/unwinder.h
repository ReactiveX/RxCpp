#pragma once

namespace unwinder { namespace detail {

        template<typename Function>
        class unwinder
        {
        public:
            ~unwinder() noexcept
            {
                if (!!function)
                {
                    (*function)();
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

            unwinder& operator=(nullptr_t) {
                dismiss();
                return *this;
            }

        private:
            unwinder();
            unwinder(const unwinder&);
            unwinder& operator=(const unwinder&);

            Function* function;
        };
} }

#define UNWIND_MAKE_IDENTIFIER_EXPLICIT_PASTER(Prefix, Suffix) Prefix ## Suffix
#define UNWIND_MAKE_IDENTIFIER_EXPLICIT(Prefix, Suffix) UNWIND_MAKE_IDENTIFIER_EXPLICIT_PASTER(Prefix, Suffix)

#define UNWIND_MAKE_IDENTIFIER(Prefix) UNWIND_MAKE_IDENTIFIER_EXPLICIT(Prefix, __LINE__)

#define ON_UNWIND(Name, Function) \
	ON_UNWIND_EXPLICIT(uwfunc_ ## Name, Name, Function)

#define ON_UNWIND_AUTO(Function) \
	ON_UNWIND_EXPLICIT(UNWIND_MAKE_IDENTIFIER(uwfunc_), UNWIND_MAKE_IDENTIFIER(unwind_), Function)

#define ON_UNWIND_EXPLICIT(FunctionName, UnwinderName, Function) \
	auto FunctionName = (Function); \
	::unwinder::detail::unwinder<decltype(FunctionName)> UnwinderName(std::addressof(FunctionName))
