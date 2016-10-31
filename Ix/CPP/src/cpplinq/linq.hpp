// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

/// 
/// namespace cpplinq
/// -----------------
///
/// Defines a number of range-based composable operators for enumerating and modifying collections
///
/// The general design philosophy is to 
///   (1) emulate the composable query patterns introduced in C# Linq
///   (2) preserve iterator category and writability where possible
/// For instance, like C# Linq we have a select operator to project one sequence into a new one.
/// Unlike Linq, invoking Select on a random access sequence will yield you a _random_ access sequence.
///
/// The general workflow begins with 'from()' which normally only takes a reference
/// the the collection in question. However, from that point on, all operators function 
/// by value, so that the range can store any necessary state, rather than duplicating it 
/// onto iterator pairs.
/// 
/// In subsequent documentation, "powers" lists which powers that the operator attempts to preserve if
/// available on on the input sequence. Some iterator powers may be skipped - in such a case, round down
/// to the next supported power (e.g. if 'fwd' and 'rnd', an input of 'bidi' will round down to a 'fwd' result). 
/// 
///  
///
/// class linq_query
/// ----------------
/// 
/// from(container&)
/// ================
/// -   Result: Query
/// 
/// Construct a new query, from an lvalue reference to a collection. Does not copy the collection
/// 
/// 
/// 
/// from(iter, iter)
/// ================
/// -   Result: Query
/// 
/// Construct a new query, from an iterator pair.
/// 
/// 
/// 
/// query.select(map)
/// ==========================
/// -   Result: Query 
/// -   Powers: input, forward, bidirectional, random access
/// 
/// For each element `x` in the input sequences, computes `map(x)` for the result sequence.
/// 
/// 
/// 
/// query.where(pred) -> query
/// ==========================
/// -   Result: Query
/// -   Powers: input, forward, bidirectional
/// 
/// Each element `x` in the input appears in the output if `pred(x)` is true.
/// 
/// The expression `pred(x)` is evaluated only when moving iterators (op++, op--). 
/// Dereferencing (op*) does not invoke the predicate.
/// 
/// 
/// 
/// query.groupby(keymap [, keyequal])
/// ====================================
/// Result: Query of groups. Each group has a 'key' field, and is a query of elements from the input.
/// Powers: forward
/// 
/// 
/// 
/// query.any([pred])
/// =================
/// -   Result: bool
/// 
/// (No argument) Returns true if sequence is non-empty. Equivalent to `query.begin()!=query.end()`
/// 
/// (One argument) Returns true if the sequence contains any elements for which `pred(element)` is true.
/// Equivalent to `query.where(pred).any()`.
/// 
/// 
/// 
/// query.all(pred)
/// ===============
/// -   Result: bool
/// 
/// Returns true if `pred` holds for all elements in the sequence. Equivalent to `!query.any(std::not1(pred))`
/// 
/// 
/// 
/// query.take(n)
/// =============
/// -   Result: query
/// -   Powers: input, forward, random access (not bidirectional)
/// 
/// Returns a sequence that contains up to `n` items from the original sequence. 
/// 
/// 
/// 
/// query.skip(n)
/// =============
/// -   Result: query
/// -   Powers: input, forward, random access (not bidirectional)
/// 
/// Returns a sequence that skips the first `n` items from the original sequence, or an empty sequence if 
/// fewer than `n` were available on input.
/// 
/// Note: begin() takes O(n) time when input iteration power is weaker than random access.
/// 
/// 
/// 
/// query.count([pred])
/// ===================
/// -   Result: std::size_t
/// 
/// _TODO: should use inner container's iterator distance type instead._
/// 
/// (Zero-argument) Returns the number of elements in the range. 
/// Equivalent to `std::distance(query.begin(), query.end())`
/// 
/// (One-argument) Returns the number of elements for whicht `pred(element)` is true.
/// Equivalent to `query.where(pred).count()`
/// 
 


#if !defined(CPPLINQ_LINQ_HPP)
#define CPPLINQ_LINQ_HPP
#pragma once

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

#include <functional>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <utility>
#include <type_traits>
#include <vector>
#include <cstddef>



// some configuration macros
#if _MSC_VER > 1600 || __cplusplus > 199711L
#define LINQ_USE_RVALUEREF 1
#endif

#if (defined(_MSC_VER) && _CPPRTTI) || !defined(_MSC_VER)
#define LINQ_USE_RTTI 1
#endif

#if defined(__clang__)
#if __has_feature(cxx_rvalue_references)
#define LINQ_USE_RVALUEREF 1
#endif
#if __has_feature(cxx_rtti)
#define LINQ_USE_RTTI 1
#endif
#endif


// individual features 
#include "util.hpp"
#include "linq_cursor.hpp"
#include "linq_iterators.hpp"
#include "linq_select.hpp"
#include "linq_take.hpp"
#include "linq_skip.hpp"
#include "linq_groupby.hpp"
#include "linq_where.hpp"
#include "linq_last.hpp"
#include "linq_selectmany.hpp"




namespace cpplinq 
{

namespace detail
{
    template<class Pred>
    struct not1_{
        Pred pred;
        not1_(Pred p) : pred(p) 
        {}
        template<class T>
        bool operator()(const T& value)
        {
            return !pred(value);
        }
    };
    // note: VC2010's std::not1 doesn't support lambda expressions. provide our own.
    template<class Pred>
    not1_<Pred> not1(Pred p) { return not1_<Pred>(p); }
}

namespace detail {
    template <class U>
    struct cast_to {
        template <class T>
        U operator()(const T& value) const {
            return static_cast<U>(value);
        }
    };
}

template <class Collection>
class linq_driver
{
    typedef typename Collection::cursor::element_type
        element_type;
    typedef typename Collection::cursor::reference_type
        reference_type;
public:
    typedef cursor_iterator<typename Collection::cursor>
        iterator;

    linq_driver(Collection c) : c(c) {}


    // -------------------- linq core methods --------------------

    template <class KeyFn>
    linq_driver< linq_groupby<Collection, KeyFn> > groupby(KeyFn fn)
    {
        return linq_groupby<Collection, KeyFn>(c, std::move(fn) );
    }

    // TODO: groupby(keyfn, eq)

    // TODO: join...

    template <class Selector>
    linq_driver< linq_select<Collection, Selector> > select(Selector sel) const {
        return linq_select<Collection, Selector>(c, std::move(sel) );
    }

    template <class Fn>
    linq_driver< linq_select_many<Collection, Fn, detail::default_select_many_selector> > 
        select_many(Fn fn) const 
    {
        return linq_select_many<Collection, Fn, detail::default_select_many_selector>(c, fn, detail::default_select_many_selector());
    }

    template <class Fn, class Fn2>
    linq_driver< linq_select_many<Collection, Fn, Fn2> > select_many(Fn fn, Fn2 fn2) const 
    {
        return linq_select_many<Collection, Fn, Fn2>(c, fn, fn2);
    }

    template <class Predicate>
    linq_driver< linq_where<Collection, Predicate> > where(Predicate p) const {
        return linq_where<Collection, Predicate>(c, std::move(p) );
    }
    

    // -------------------- linq peripheral methods --------------------

    template <class Fn>
    element_type aggregate(Fn fn) const
    {
        auto it = begin();
        if (it == end()) {
            return element_type();
        }
        
        reference_type first = *it;
        return std::accumulate(++it, end(), first, fn);
    }

    template <class T, class Fn>
    T aggregate(T initialValue, Fn fn) const
    {
        return std::accumulate(begin(), end(), initialValue, fn);
    }

    bool any() const { auto cur = c.get_cursor(); return !cur.empty(); }

    template <class Predicate>
    bool any(Predicate p) const {
        auto it = std::find_if(begin(), end(), p);
        return it != end();
    }

    template <class Predicate>
    bool all(Predicate p) const {
        auto it = std::find_if(begin(), end(), detail::not1(p));
        return it == end();
    }

    // TODO: average

#if !defined(__clang__)
    // Clang complains that linq_driver is not complete until the closing brace 
    // so (linq_driver*)->select() cannot be resolved.
    template <class U>
    auto cast() 
    -> decltype(static_cast<linq_driver*>(0)->select(detail::cast_to<U>())) 
    {
        return this->select(detail::cast_to<U>());
    }
#endif

    // TODO: concat

    bool contains(const typename Collection::cursor::element_type& value) const {
        return std::find(begin(), end(), value) != end();
    }

    typename std::iterator_traits<iterator>::difference_type count() const {
        return std::distance(begin(), end());
    }

    template <class Predicate>
    typename std::iterator_traits<iterator>::difference_type count(Predicate p) const {
        auto filtered = this->where(p);
        return std::distance(begin(filtered), end(filtered));
    }

    // TODO: default_if_empty
    
    // TODO: distinct()
    // TODO: distinct(cmp)

    reference_type element_at(std::size_t ix) const {
        auto cur = c.get_cursor();
        while(ix && !cur.empty()) {
            cur.inc();
            --ix;
        }
        if (cur.empty()) { throw std::logic_error("index out of bounds"); }
        else             { return cur.get(); }
    }

    element_type element_at_or_default(std::size_t ix) const {
        auto cur = c.get_cursor();
        while(ix && !cur.empty()) {
            cur.inc();
            -- ix;
        }
        if (cur.empty()) { return element_type(); }
        else             { return cur.get(); }
    }

    bool empty() const {
        return !this->any();
    }

    // TODO: except(second)
    // TODO: except(second, eq)

    reference_type first() const {
        auto cur = c.get_cursor();
        if (cur.empty()) { throw std::logic_error("index out of bounds"); }
        else             { return cur.get(); }
    }

    template <class Predicate>
    reference_type first(Predicate pred) const {
        auto cur = c.get_cursor();
        while (!cur.empty() && !pred(cur.get())) {
            cur.inc();
        }
        if (cur.empty()) { throw std::logic_error("index out of bounds"); }
        else             { return cur.get(); }
    }

    element_type first_or_default() const {
        auto cur = c.get_cursor();
        if (cur.empty()) { return element_type(); }
        else             { return cur.get(); }
    }

    template <class Predicate>
    element_type first_or_default(Predicate pred) const {
        auto cur = c.get_cursor();
        while (!cur.empty() && !pred(cur.get())) {
            cur.inc();
        }
        if (cur.empty()) { return element_type(); }
        else             { return cur.get(); }
    }
    
    // TODO: intersect(second)
    // TODO: intersect(second, eq)

    // note: forward cursors and beyond can provide a clone, so we can refer to the element directly
    typename std::conditional< 
        std::is_convertible<
            typename Collection::cursor::cursor_category,
            forward_cursor_tag>::value,
        reference_type,
        element_type>::type
    last() const 
    {
        return linq_last_(c.get_cursor(), typename Collection::cursor::cursor_category());
    }

    template <class Predicate>
    reference_type last(Predicate pred) const 
    {
        auto cur = c.where(pred).get_cursor();
        return linq_last_(cur, typename decltype(cur)::cursor_category());
    }

    element_type last_or_default() const 
    {
        return linq_last_or_default_(c.get_cursor(), typename Collection::cursor::cursor_category());
    }

    template <class Predicate>
    element_type last_or_default(Predicate pred) const 
    {
        auto cur = c.where(pred).get_cursor();
        return linq_last_or_default_(cur, typename decltype(cur)::cursor_category());
    }

    reference_type max() const
    {
        return max(std::less<element_type>());
    }

    template <class Compare>
    reference_type max(Compare less) const
    {
        auto it = std::max_element(begin(), end(), less);
        if (it == end()) 
            throw std::logic_error("max performed on empty range");

        return *it;
    }

    reference_type min() const
    {
        return min(std::less<element_type>());
    }

    template <class Compare>
    reference_type min(Compare less) const
    {
        auto it = std::min_element(begin(), end(), less);
        if (it == end()) 
            throw std::logic_error("max performed on empty range");

        return *it;
    }

    // TODO: order_by(sel)
    // TODO: order_by(sel, less)
    // TODO: order_by_descending(sel)
    // TODO: order_by_descending(sel, less)

    // TODO: sequence_equal(second)
    // TODO: sequence_equal(second, eq)

    // TODO: single / single_or_default

    linq_driver<linq_skip<Collection>> skip(std::size_t n) const {
        return linq_skip<Collection>(c, n);
    }

    // TODO: skip_while(pred)

    template<typename ITEM = typename element_type>
    typename std::enable_if<std::is_default_constructible<ITEM>::value, ITEM>::type sum() const {
        ITEM seed{};
        return sum(seed);
    }

    typename element_type sum(typename element_type seed) const {
        return std::accumulate(begin(), end(), seed);
    }

    template <typename Selector, typename Result = std::result_of<Selector(typename element_type)>::type>
    typename std::enable_if<std::is_default_constructible<Result>::value, Result>::type sum(Selector sel) const {
        return from(begin(), end()).select(sel).sum();			
    }

    template <typename Selector, typename Result = std::result_of<Selector(typename element_type)>::type>
    Result sum(Selector sel, Result seed) const {
        return from(begin(), end()).select(sel).sum(seed);			
    }

    linq_driver<linq_take<Collection>> take(std::size_t n) const {
        return linq_take<Collection>(c, n);
    }

    // TODO: take_while

    // TODO: then_by / then_by_descending ?

    // TODO: to_...

    // TODO: union(second)
    // TODO: union(eq)

    // TODO: zip
    
    // -------------------- conversion methods --------------------

    std::vector<typename Collection::cursor::element_type> to_vector() const 
    {
        return std::vector<typename Collection::cursor::element_type>(begin(), end());
    }

    std::list<typename Collection::cursor::element_type> to_list() const
    {
        return std::list<typename Collection::cursor::element_type>(begin(), end());
    }

    std::set<typename Collection::cursor::element_type> to_set() const
    {
        return std::set<typename Collection::cursor::element_type>(begin(), end());
    }

    // -------------------- container/range methods --------------------

    iterator begin() const  { auto cur = c.get_cursor(); return !cur.empty() ? iterator(cur) : iterator(); }
    iterator end() const    { return iterator(); }
    linq_driver& operator=(const linq_driver& other) { c = other.c; return *this; }
    template <class TC2> 
    linq_driver& operator=(const linq_driver<TC2>& other) { c = other.c; return *this; }

    typename std::iterator_traits<iterator>::reference
        operator[](std::size_t ix) const {
        return *(begin()+=ix);
    }

    // -------------------- collection methods (leaky abstraction) --------------------

    typedef typename Collection::cursor cursor;
    cursor get_cursor() { return c.get_cursor(); }

    linq_driver< dynamic_collection<typename Collection::cursor::reference_type> >
        late_bind() const
    {
        return dynamic_collection<typename Collection::cursor::reference_type>(c);
    }

private: 
    Collection c;
};
 
// TODO: should probably use reference-wrapper instead? 
template <class TContainer>
linq_driver<iter_cursor<typename util::container_traits<TContainer>::iterator>> from(TContainer& c)
{ 
    auto cur = iter_cursor<typename util::container_traits<TContainer>::iterator>(std::begin(c), std::end(c));
    return cur;
}
template <class T>
const linq_driver<T>& from(const linq_driver<T>& c) 
{ 
    return c; 
}
template <class Iter>
linq_driver<iter_cursor<Iter>> from(Iter start, Iter finish)
{
    return iter_cursor<Iter>(start, finish);
}

template <class TContainer>
linq_driver<TContainer> from_value(const TContainer& c)
{ 
    return linq_driver<TContainer>(c);
}

}

#pragma pop_macro("min")
#pragma pop_macro("max")

#endif // defined(CPPLINQ_LINQ_HPP)

