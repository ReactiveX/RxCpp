// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_UTIL_HPP)
#define CPPLINQ_LINQ_UTIL_HPP
#pragma once

namespace cpplinq { namespace util {

    template <class Container>
    struct container_traits {
        typedef typename Container::iterator iterator;
        typedef typename std::iterator_traits<iterator>::value_type value_type;
        typedef typename std::iterator_traits<iterator>::iterator_category iterator_category;

        // TODO: conservative definition for now. 
        enum { is_writable_iterator = 
                   std::is_reference<typename std::iterator_traits<iterator>::reference>::value
                   && std::is_same<typename std::remove_cv<value_type>::type,
                                   typename std::remove_cv<typename std::remove_reference<typename std::iterator_traits<iterator>::reference>::type>::type>::value
        };
    };

    template <>
    struct container_traits<int>;

    template <class Container>
    struct container_traits<Container&>
        : container_traits<Container>
    {};
    template <class Container>
    struct container_traits<const Container>
        : container_traits<Container>
    {
        typedef typename Container::const_iterator iterator;
    };

    // Note: returns false if no partial order exists between two 
    // particular iterator categories, such as with some of the boost categories
    template <class Cat1, class Cat2>
    struct less_or_equal_iterator_category
    {
    private:
        typedef char yes;
        typedef struct { char c1,c2; } no;
        static yes invoke(Cat1);
        static no invoke(...);
    public:
        enum { value = (sizeof(invoke(Cat2())) == sizeof(yes)) };
    };

    // Return the weaker of the two iterator categories. Make sure 
    //   a non-standard category is in the second argument position, as 
    //   this metafunction will default to the first value if the order is undefined
    template <class Cat1, class Cat2>
    struct min_iterator_category
        : std::conditional<
            less_or_equal_iterator_category<Cat2, Cat1>::value,
            Cat2,
            Cat1>
    {
    };

#if 0
#define CppLinq_GET_ITERATOR_TYPE(TContainer) \
    decltype(begin(static_cast<TContainer*>(0)))
#define CppLinq_GET_CONST_ITERATOR_TYPE(TContainer) \
    decltype(begin(static_cast<const TContainer*>(0)))
#else
#define CppLinq_GET_ITERATOR_TYPE(TContainer) \
    typename ::cpplinq::util::container_traits<TContainer>::iterator
#define CppLinq_GET_CONST_ITERATOR_TYPE(TContainer) \
    typename ::cpplinq::util::container_traits<TContainer>::const_iterator
#endif

    // VC10's std::tr1::result_of is busted with lambdas. use decltype instead on vc10 and later
#if defined(_MSC_VER) && _MSC_VER >= 1600
    namespace detail {
        template <class T> T instance();
    };
    template <class Fn> struct result_of;
    template <class Fn>
    struct result_of<Fn()> {
        typedef decltype(detail::instance<Fn>()()) type;
    };
    template <class Fn, class A0>
    struct result_of<Fn(A0)> {
        typedef decltype(detail::instance<Fn>()(detail::instance<A0>())) type;
    };
    template <class Fn, class A0, class A1>
    struct result_of<Fn(A0,A1)> {
        typedef decltype(detail::instance<Fn>()(detail::instance<A0>(),
                                                detail::instance<A1>())) type;
    };
    template <class Fn, class A0, class A1, class A2>
    struct result_of<Fn(A0,A1,A2)> {
        typedef decltype(detail::instance<Fn>()(detail::instance<A0>(),
                                                detail::instance<A1>(),
                                                detail::instance<A2>())) type;
    };
    template <class Fn, class A0, class A1, class A2, class A3>
    struct result_of<Fn(A0,A1,A2,A3)> {
        typedef decltype(detail::instance<Fn>()(detail::instance<A0>(),
                                                detail::instance<A1>(),
                                                detail::instance<A2>(),
                                                detail::instance<A3>())) type;
    };
#elif defined(_MSC_VER)
    template <class T>
    struct result_of<T> : std::tr1::result_of<T> {};
#else
    using std::result_of;
#endif

    template<class Type>
    struct identity 
    {
        typedef Type type;
        Type operator()(const Type& left) const {return left;}
    };

    // faux pointer proxy for iterators that dereference to a value rather than reference, such as selectors
    template <class T>
    struct value_ptr
    {
        T value;
        value_ptr(const T& value) : value(value)
        {}
        value_ptr(const T* pvalue) : value(*pvalue)
        {}
        const T* operator->()
        {
            return &value;
        }
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

        void set(T value) {
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
}}


#endif //CPPLINQ_UTIL_HPP

