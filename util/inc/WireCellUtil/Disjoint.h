#ifndef WIRECELLUTIL_DISJOINT
#define WIRECELLUTIL_DISJOINT

#include "WireCellUtil/Exceptions.h"

#include <boost/iterator/iterator_adaptor.hpp>

#include <functional>           // reference_wrapper
#include <vector>
#include <map>                  // pair

namespace WireCell {


    template <typename OuterIter
              , typename InnerIter = typename OuterIter::value_type::iterator
              , typename Value = typename InnerIter::value_type
              , typename Reference = Value&
              >
    class disjoint_iterator :
        public boost::iterator_facade<disjoint_iterator<OuterIter>
                                      // value
                                      , Value
                                      // cagegory
                                      , boost::random_access_traversal_tag
                                      // reference
                                      , Reference
                                      >
    {
      private:
        using base_type = boost::iterator_facade<disjoint_iterator<OuterIter>
                                                 , Value
                                                 , boost::random_access_traversal_tag
                                                 , Reference
                                                 >;

        using outer_iterator = OuterIter;
        using inner_iterator = InnerIter;

        struct inner_range_type {
            outer_iterator outer;
            // Number of inners in range
            size_t size() const { return std::distance(begin(), end()); }
            inner_iterator begin() const {
                auto it = outer->begin();
                return it;
            }
            inner_iterator end() const { 
                auto it = outer->end();
                return it;
            }
            bool empty() const { return begin() == end(); }
        };
        using accum_map = std::map<size_t, inner_range_type>;
        using accum_iterator = typename accum_map::iterator;
        struct cursor_type {
            accum_iterator accum{}; // -> {Naccum, inner range}
            inner_iterator inner{}; // current inner iterator in range
            size_t index{0};        // corresponding global index comparable to size()

            bool operator==(const cursor_type& o) {
                return accum == o.accum
                    and inner == o.inner;
            }

            bool at_inner_end() const {
                return inner == accum->second.end();
            }
        };

        // Index and order by how many inners came before a range.
        accum_map accums;
        // Where we are in the accumulation
        cursor_type cursor;

        // total number of inners
        size_t inners_size_{0};

      public:
        using difference_type = typename base_type::difference_type;
        using value_type = typename base_type::value_type;
        using pointer = typename base_type::value_type*;
        using reference = typename base_type::value_type&;

        // Construct an "end" iterator.
        disjoint_iterator()
        {
            cursor.accum = accums.end();
            cursor.index = 0;       // equal to size
        }
        disjoint_iterator(OuterIter end)
        {
            cursor.accum = accums.end();
            cursor.index = 0;       // equal to size
        }

        // Construct a full iterator.
        disjoint_iterator(OuterIter beg, OuterIter end)
        {
            for (auto it = beg; it != end; ++it) {
                inner_range_type ir{it};
                if (ir.empty()) { continue; }
                accums[inners_size_] = ir;
                inners_size_ += ir.size();
            }

            if (inners_size_ == 0) {
                cursor.accum = accums.end();
                cursor.index = 0;       // equal to size
                return;
            }

            // initialize cursor at start
            cursor.accum = accums.begin();
            cursor.inner = cursor.accum->second.begin();
            cursor.index = 0;
        }

        // Give range API
        disjoint_iterator begin() const {
            return *this;
        }
        disjoint_iterator end() const {
            return disjoint_iterator();
        }

        // Size of all inners
        size_t size() const { return inners_size_; }

      private:

        friend class boost::iterator_core_access;

        bool at_outer_end() const {
            return cursor.accum == accums.end();
        }

        bool at_outer_begin() const {
            return cursor.accum == accums.begin()
                and cursor.inner == cursor.accum->second.begin();
        }

        template <typename OtherOuterIter>
        bool equal(disjoint_iterator<OtherOuterIter> const& x) const {
            if (at_outer_end() and x.at_outer_end()) return true;
            if (at_outer_end() or x.at_outer_end()) return false;
            // both are non-end
            return cursor.accum == x.cursor.accum
                and cursor.inner == x.cursor.inner;
        }

        void increment()
        {
            if (accums.empty()) {
                throw std::runtime_error("incrementing empty outer");
            }

            if (at_outer_end()) {
                throw std::runtime_error("increment past end");
            }

            ++cursor.inner;
            ++cursor.index;

            if (cursor.at_inner_end()) {
                // go next cursor
                ++cursor.accum;

                if (at_outer_end()) { // fell off
                    cursor.inner = inner_iterator{};
                    cursor.index = size();
                    return;
                }
                // at start of non-empty range
                cursor.inner = cursor.accum->second.begin();
                return;
            }
        }

        void decrement()
        {
            if (accums.empty()) {
                throw std::runtime_error("decrementing empty outer");
            }

            if (at_outer_begin()) {
                throw std::runtime_error("decrementing past begin");
            }

            // If we are one past an outer
            if (cursor.inner == cursor.accum->second.begin()
                or at_outer_end()) {
                // reverse by one outer.  must work even if we were at
                // outer_end.
                --cursor.accum;
                // inner starts at end
                cursor.inner = cursor.accum->second.end();
            }
            --cursor.inner;
            --cursor.index;
        }

        void advance(difference_type n)
        {
            // Fixme: this method can be optimized by using the
            // cursor.accum.first and cursor.accum.size() to walk
            // outer itterators until landing on the one holding the
            // target distance.
            if (n>0) {
                while (n) {
                    ++(*this);
                    --n;
                }
                return;
            }
            if (n<0) {
                while (n) {
                    --(*this);
                    ++n;
                }
                return;
            }
        }

        difference_type distance_to(disjoint_iterator<OuterIter> const& other) const
        {
            return other.cursor.index - this->cursor.index;
        }

        reference dereference() const
        {
            return *cursor.inner;
        }
    };                              // disjoint_iterator



    template<typename OuterIter>
    struct disjoint_range {
        using iterator = disjoint_iterator<OuterIter>;

        OuterIter b, e;
        iterator begin() const { return iterator(b,e); }
        iterator end() const { return iterator(); }
    };

    template <typename OuterIter>
    auto flatten(OuterIter end) -> disjoint_iterator<OuterIter>
    {
        return disjoint_iterator<OuterIter>(end);
    }
    template <typename OuterIter>
    auto flatten(OuterIter begin, OuterIter end) -> disjoint_iterator<OuterIter>
    {
        return disjoint_iterator<OuterIter>(begin, end);
    }

}

#endif
