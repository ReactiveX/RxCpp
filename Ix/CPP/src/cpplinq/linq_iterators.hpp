// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_ITERATORS_HPP)
#define CPPLINQ_LINQ_ITERATORS_HPP
#pragma once

#include <cstddef>

namespace cpplinq {

    // if a member, provides the straightforward implementation of various redundant operators. For example,
    //   providing -> for any iterator providing *, and so forth.
    struct use_default_iterator_operators {};

    #define CPPLINQ_USE_DEFAULT_ITERATOR_OPERATORS \
    operator ::cpplinq::use_default_iterator_operators() const { return ::cpplinq::use_default_iterator_operators(); }

    template <class Iter>
    typename std::enable_if<
        std::is_convertible<Iter, use_default_iterator_operators>::value,
        Iter
        >::type
    operator+(const Iter& it, typename std::iterator_traits<Iter>::distance_type n) {
        return it += n;
    }
    template <class Iter>
    typename std::enable_if<
        std::is_convertible<Iter, use_default_iterator_operators>::value,
        Iter
        >::type
    operator-(const Iter& it, typename std::iterator_traits<Iter>::distance_type n) {
        return it -= n;
    }
    template <class Iter>
    typename std::enable_if<
        std::is_convertible<Iter, use_default_iterator_operators>::value,
        Iter
        >::type
    operator-=(const Iter& it, typename std::iterator_traits<Iter>::distance_type n) {
        return it += (-n);
    }

    template <class Iter>
    typename std::enable_if<
        std::is_convertible<Iter, use_default_iterator_operators>::value,
        bool
        >::type
    operator!=(const Iter& it, const Iter& it2) {
        return !(it == it2);
    }
    template <class Iter>
    typename std::enable_if<
        std::is_convertible<Iter, use_default_iterator_operators>::value,
        bool
        >::type
    operator>(const Iter& it, const Iter& it2) {
        return it2 < it;
    }
    template <class Iter>
    typename std::enable_if<
        std::is_convertible<Iter, use_default_iterator_operators>::value,
        bool
        >::type
    operator<=(const Iter& it, const Iter& it2) {
        return !(it2 < it);
    }
    template <class Iter>
    typename std::enable_if<
        std::is_convertible<Iter, use_default_iterator_operators>::value,
        bool
        >::type
    operator>=(const Iter& it, const Iter& it2) {
        return !(it < it2);
    }   
    
    namespace util {
        template <class Iter, class T>
        typename std::iterator_traits<Iter>::pointer deref_iterator(const Iter& it) {
            return deref_iterator(it, util::identity<typename std::iterator_traits<Iter>::reference>());
        }

        template <class Iter, class T>
        T* deref_iterator(const Iter& it, util::identity<T&>) {
            return &*it;
        }

        template <class Iter, class T>
        util::value_ptr<T> deref_iterator(const Iter& it, util::identity<T>) {
            return util::value_ptr<T>(*it);
        }
    } 
    
    
    template <class Iter>
    class iter_range
    {
        Iter start, finish;
    public:

        CPPLINQ_USE_DEFAULT_ITERATOR_OPERATORS

        typedef Iter iterator;
        typedef typename iterator::value_type value_type;

        explicit iter_range(Iter start, Iter finish) : start(start), finish(finish) {}
        iterator begin() const { return start; }
        iterator end() const { return finish; }
    };
    template <class Iter>
    iter_range<Iter> make_range(Iter start, Iter finish) {
        return iter_range<Iter>(start, finish);
    }

    // decays into a onepass/forward iterator
    template <class Cursor>
    class cursor_iterator 
        : public std::iterator<std::forward_iterator_tag, 
                typename Cursor::element_type,
                std::ptrdiff_t,
                typename std::conditional<std::is_reference<typename Cursor::reference_type>::value,
                                          typename std::add_pointer<typename Cursor::element_type>::type,
                                          util::value_ptr<typename Cursor::element_type>>::type,
                typename Cursor::reference_type>
    {
    public:
        CPPLINQ_USE_DEFAULT_ITERATOR_OPERATORS;

        cursor_iterator(Cursor cur) : cur(cur) {
        }

        cursor_iterator() : cur() {
        }

        bool operator==(const cursor_iterator& other) const {
            return !cur && !other.cur;
        }

        typename Cursor::reference_type operator*() const {
            return cur->get();
        }

        typename cursor_iterator::pointer operator->() const {
            auto& v = **this;
            return &v;
        }
        
        cursor_iterator& operator++() {
            cur->inc();

            if (cur->empty()) { cur.reset(); }
            return *this;
        }

        cursor_iterator& operator+=(std::ptrdiff_t n) {
            cur->skip(n);

            if (cur->empty()) { cur.reset(); }
            return *this;
        }


        
    private:
        bool empty() const {
            !cur || cur->empty();
        }

        util::maybe<Cursor> cur;
    };

    template <class Container>
    class container_range
    {
        Container c;

    public:
        typedef cursor_iterator<typename Container::cursor> iterator;

        container_range(Container c) : c(c) 
        {
        }

        iterator begin() const
        {
            return iterator(c.get_cursor());
        }

        iterator end() const
        {
            return iterator();
        }
    };

}

#endif
