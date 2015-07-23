// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_CURSOR_HPP)
#define CPPLINQ_LINQ_CURSOR_HPP
#pragma once

#include <cstddef>

/// cursors
/// ----------
/// It should be noted that CppLinq uses a slightly different iterator concept, one where iterators
/// know their extents. This sacrificed some generality, but it adds convenience and improves
/// some performance in some cases. Notably, captures need only be stored once instead of twice in 
/// most use cases.
/// 
/// Cursors and Ranges are always classes.
/// 
/// To get a cursor from a range:
/// 
///    get_cursor(range) -> cur
/// 
/// Unlike boost ranges, CppLinq cursors are mutated directly, and may "shed state" as they are
/// mutated. For example, a GroupBy range will drop references to earlier groups, possibly 
/// permitting freeing them.
/// 
/// Onepass cursor
/// ===========
/// -   empty(cur) -> bool  : at end of sequence
/// -   inc(cur) 
/// -   get(cur) -> T 
/// -   copy ctor           : duplicate reference to seek position
/// 
/// Forward cursor
/// =============
/// -   copy ctor           : true duplicate of seek position
/// 
/// Bidirectional cursor
/// ====================
/// -   forget()            : notes the current element as the new 'begin' point
/// -   atbegin(cur) -> bool
/// -   dec(cur)
/// 
/// Random access cursor
/// ====================
/// -   skip(cur, n)
/// -   position(cur) -> n
/// -   size(cur)     -> n
/// -   truncate(n)         : keep only n more elements
/// 
/// As well, cursors must define the appropriate type/typedefs:
/// -   cursor_category  :: { onepass_cursor_tag, forward_cursor_tag, bidirectional_cursor_tag, random_access_cursor_tag }
/// -   element_type
/// -   reference_type   : if writable, element_type& or such. else, == element_type
/// -   



namespace cpplinq { 

    // used to identify cursor-based collections
    struct collection_tag {};

    struct onepass_cursor_tag {};
    struct forward_cursor_tag : onepass_cursor_tag  {};
    struct bidirectional_cursor_tag : forward_cursor_tag {}; 
    struct random_access_cursor_tag : bidirectional_cursor_tag {};

    struct noread_cursor_tag {}; // TODO: remove if not used
    struct readonly_cursor_tag : noread_cursor_tag {};
    struct readwrite_cursor_tag : readonly_cursor_tag {};



    // standard cursor adaptors

    namespace util 
    {
        namespace detail 
        {
            template <std::size_t n> struct type_to_size { char value[n]; };

            type_to_size<1> get_category_from_iterator(std::input_iterator_tag);
            type_to_size<2> get_category_from_iterator(std::forward_iterator_tag);
            type_to_size<3> get_category_from_iterator(std::bidirectional_iterator_tag);
            type_to_size<4> get_category_from_iterator(std::random_access_iterator_tag);
        }

        template <std::size_t>
        struct iter_to_cursor_category_;

        template <class Iter>
        struct iter_to_cursor_category
        {
            static const std::size_t catIx = sizeof(detail::get_category_from_iterator(typename std::iterator_traits<Iter>::iterator_category()) /*.value*/  );
            typedef typename iter_to_cursor_category_<catIx>::type type;
        };

        template <> struct iter_to_cursor_category_<1> { typedef onepass_cursor_tag type; };
        template <> struct iter_to_cursor_category_<2> { typedef forward_cursor_tag type; };
        template <> struct iter_to_cursor_category_<3> { typedef bidirectional_cursor_tag type; };
        template <> struct iter_to_cursor_category_<4> { typedef random_access_cursor_tag type; };


        // Note: returns false if no partial order exists between two 
        // particular iterator categories, such as with some of the boost categories
        template <class Cat1, class Cat2>
        struct less_or_equal_cursor_category
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
        template <class Cat1, class Cat2, class Cat3 = void>
        struct min_cursor_category : min_cursor_category<typename min_cursor_category<Cat1, Cat2>::type, Cat3>
        {
        };

        template <class Cat1, class Cat2>
        struct min_cursor_category<Cat1, Cat2>
            : std::conditional<
                less_or_equal_cursor_category<Cat2, Cat1>::value,
                Cat2,
                Cat1>
        {
        };

        template <class Collection>
        struct cursor_type {
            typedef decltype(cursor(*static_cast<Collection*>(0))) type;
        };
    }
    
    // simultaniously models a cursor and a cursor-collection
    template <class Iterator>
    class iter_cursor : collection_tag {
    public:

        typedef iter_cursor cursor;

        typedef typename std::remove_reference<typename std::iterator_traits<Iterator>::value_type>::type
            element_type;
        typedef typename std::iterator_traits<Iterator>::reference
            reference_type;
        typedef typename util::iter_to_cursor_category<Iterator>::type
            cursor_category;

        void forget() { start = current; }
        bool empty() const { return current == fin; }
        void inc() { 
            if (current == fin)
                throw std::logic_error("inc past end");
            ++current; 
        }
        typename std::iterator_traits<Iterator>::reference get() const { return *current; }

        bool atbegin() const { return current == start; }
        void dec() { 
            if (current == start) 
                throw std::logic_error("dec past begin");
            --current; 
        }
        
        void skip(std::ptrdiff_t n) { current += n; }
        std::size_t size() { return fin-start; }
        void position() { return current-start; }
        void truncate(std::size_t n) {
            if (n > fin-current) {
                fin = current + n;
            }
        }


        iter_cursor(Iterator start, Iterator fin)
        : current(start)
        , start(start)
        , fin(std::move(fin))
        {
        }

        iter_cursor(Iterator start, Iterator fin, Iterator current)
        : current(std::move(current))
        , start(std::move(start))
        , fin(std::move(fin))
        {
        }

        iter_cursor get_cursor() const { return *this; }

    private:
        Iterator current;
        Iterator start, fin;
    };


    template <class T>
    struct cursor_interface
    {
        virtual bool empty() const = 0;
        virtual void inc() = 0;
        virtual cursor_interface* copy() const = 0;

        virtual T get() const = 0;

        virtual ~cursor_interface() {}
    };

    template <class T>
    class dynamic_cursor : collection_tag
    {
        template <class Cur>
        struct instance : cursor_interface<T>
        {
            Cur innerCursor;

            instance(Cur cursor) : innerCursor(std::move(cursor))
            {
            }
            virtual bool empty() const
            {
                return innerCursor.empty();
            }
            virtual void inc()
            {
                innerCursor.inc();
            }
            virtual T get() const 
            {
                return innerCursor.get();
            }
            virtual cursor_interface<T>* copy() const 
            {
                return new instance(*this);
            }
        };

        std::unique_ptr<cursor_interface<T>> myCur;

    public:
        typedef forward_cursor_tag cursor_category; // TODO: not strictly true!
        typedef typename std::remove_reference<T>::type element_type;
        typedef T reference_type;

        dynamic_cursor() {}

        dynamic_cursor(const dynamic_cursor& other)
        : myCur(other.myCur ? other.myCur->copy() : nullptr)
        {
        }

        dynamic_cursor(dynamic_cursor&& other)
        : myCur(other.myCur.release())
        {
        }

        template <class Cursor>
        dynamic_cursor(Cursor cursor) 
        : myCur(new instance<Cursor>(std::move(cursor)))
        { 
        }

        template <class Iterator>
        dynamic_cursor(Iterator start, Iterator end)
        {
            *this = iter_cursor<Iterator>(start, end);
        }

        bool empty() const { return !myCur || myCur->empty(); }
        void inc() { myCur->inc(); }
        T get() const { return myCur->get(); }

        dynamic_cursor& operator=(dynamic_cursor other)
        {
            std::swap(myCur, other.myCur);
            return *this;
        }
    };

    template <class T>
    struct container_interface
    {
        virtual dynamic_cursor<T> get_cursor() const = 0;
    };

    template <class T>
    class dynamic_collection
    {
        std::shared_ptr< container_interface<T> > container;

        template <class Container>
        struct instance : container_interface<T>
        {
            Container c;

            instance(Container c) : c(c)
            {
            }

            dynamic_cursor<T> get_cursor() const
            {
                return c.get_cursor();
            }
        };

    public:
        typedef dynamic_cursor<T> cursor;
        
        dynamic_collection() {}

        dynamic_collection(const dynamic_collection& other) 
        : container(other.container) 
        {
        }

        // container or query
        template <class Container>
        dynamic_collection(Container c) 
        : container(new instance<Container>(c))
        {
        }

        // container or query
        template <class Iterator>
        dynamic_collection(Iterator begin, Iterator end) 
        : container(new instance< iter_cursor<Iterator> >(iter_cursor<Iterator>(begin, end)))
        {
        }
        
        dynamic_cursor<T> get_cursor() const {
            return container ? container->get_cursor() : dynamic_cursor<T>();
        }
    };
}

#endif // !defined(CPPLINQ_LINQ_CURSOR_HPP
