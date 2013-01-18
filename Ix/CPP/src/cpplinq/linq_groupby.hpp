// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPLINQ_LINQ_GROUPBY_HPP)
#define CPPLINQ_LINQ_GROUPBY_HPP
#pragma once

namespace cpplinq 
{

template <class Iter, class Key>
struct group
{
    Key key;
    Iter start;
    Iter fin;

    typedef Iter iterator;
    typedef Iter const_iterator;

    group(){}

    group(const Key& key) : key(key)
    {
    }

    Iter begin() const { return start; }
    Iter end() const { return fin; }
};

struct default_equality
{
    template <class T>
    bool operator()(const T& a, const T& b) const {
        return a == b;
    }
};
struct default_less
{
    template<class T>
    bool operator()(const T& a, const T& b) const {
        return a < b;
    }
};

// progressively constructs grouping as user iterates over groups and elements
//   within each group. Performs this task by building a std::list of element 
//   iterators with equal elements within each group.
// 
// invariants:
//   - relative order of groups corresponds to relative order of each group's first 
//     element, as they appeared in the input sequence.
//   - relative order of elements within a group correspond to relative order
//     as they appeared in the input sequence.
// 
// requires:
//   Iter must be a forward iterator.
template <class Collection, class KeyFn, class Compare = default_less>
class linq_groupby
{
    typedef typename Collection::cursor 
        inner_cursor;

    typedef typename util::result_of<KeyFn(typename inner_cursor::element_type)>::type
        key_type;

    typedef std::list<typename inner_cursor::element_type>
        element_list_type;

    typedef group<typename element_list_type::iterator, key_type> 
        group_type;

    typedef std::list<group_type>
        group_list_type;

private:
    struct impl_t
    {
        // TODO: would be faster to use a chunked list, where
        //   pointers are invalidated but iterators are not. Need 
        //   benchmarks first

        element_list_type                           elements;
        std::list<group_type>                       groups;
        std::map<key_type, group_type*, Compare>    groupIndex;



        KeyFn keySelector;
        Compare comp;
        
        impl_t(inner_cursor cur,
               KeyFn keySelector,
               Compare comp = Compare()) 
        : keySelector(keySelector)
        , groupIndex(comp)
        {
            // TODO: make lazy
            insert_all(std::move(cur));
        }

        void insert_all(inner_cursor cur)
        {
            while(!cur.empty()) {
                insert(cur.get());
                cur.inc();
            }
        }
        void insert(typename inner_cursor::reference_type element)
        {
            key_type key = keySelector(element);
            auto groupPos = groupIndex.find(key);
            if(groupPos == groupIndex.end()) {
                // new group
                bool firstGroup = groups.empty();

                elements.push_back(element);
                if(!firstGroup) {
                    // pop new element out of previous group
                    --groups.back().fin; 
                }

                // build new group
                groups.push_back(group_type(key));
                group_type& newGroup = groups.back();

                groupIndex.insert( std::make_pair(key, &newGroup) );

                newGroup.fin = elements.end();
                --(newGroup.start = newGroup.fin);
            } else {
                // add to existing group at end
                elements.insert(groupPos->second->end(), element);
            }
        }
    };

public:
    struct cursor {
        typedef group_type
            element_type;

        typedef element_type
            reference_type;

        typedef forward_cursor_tag
            cursor_category;

        cursor(inner_cursor   cur, 
               KeyFn          keyFn,
               Compare        comp = Compare()) 
        {
            impl.reset(new impl_t(cur, keyFn, comp));
            inner   = impl->groups.begin();
            fin     = impl->groups.end();
        }

        void forget() { } // nop on forward-only cursors
        bool empty() const {
            return inner == fin;
        }
        void inc() {
            if (inner == fin) {
                throw std::logic_error("attempt to iterate past end of range");
            }
            ++inner;
        }
        reference_type get() const {
            return *inner;
        }
        
    private:
        std::shared_ptr<impl_t> impl;
        typename std::list<group_type>::iterator inner;
        typename std::list<group_type>::iterator fin;
    };

    linq_groupby(Collection     c, 
                 KeyFn          keyFn,
                 Compare        comp = Compare()) 
    : c(c), keyFn(keyFn), comp(comp)
    {
    }

    cursor get_cursor() const { return cursor(c.get_cursor(), keyFn, comp); }

private:
    Collection c;
    KeyFn keyFn;
    Compare comp;
};

}

#endif // !defined(CPPLINQ_LINQ_GROUPBY_HPP)

