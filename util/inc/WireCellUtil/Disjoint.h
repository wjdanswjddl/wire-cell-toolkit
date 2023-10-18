#ifndef WIRECELLUTIL_DISJOINT
#define WIRECELLUTIL_DISJOINT

#include "WireCellUtil/Exceptions.h"

#include <boost/iterator/iterator_adaptor.hpp>

#include <functional>           // reference_wrapper
#include <vector>
#include <map>                  // pair

namespace WireCell {


    /** Disjoint.

        A disjoint models an iterator and an iterator range over a
        collection-of-collections that presents as an iterator and an
        iterator range over a flat monolithic collection.

        Eg, a std::vector<std::map<int,string>> may be accessed via a
        disjoint as a std::vector<int>.  In this example, the std::vector
        is the "outer" collection and the std::map is the "inner"
        collection.

        If the outer iterator type provides standard STL types then
        the inner iterator type is infered.

        It is the callers responsibility to maintain the lifetime of the
        original collection of collections as disjoint deals only with
        iterators.
    */
    template <typename OuterIter
              , typename InnerIter = typename OuterIter::value_type::iterator
              , typename Value = typename InnerIter::value_type
              , typename Reference = Value&
              >
    class disjoint :
        public boost::iterator_facade<disjoint<OuterIter>
                                      // value
                                      , Value
                                      // cagegory
                                      , boost::random_access_traversal_tag
                                      // reference
                                      , Reference
                                      >
    {
      private:
        using base_type = boost::iterator_facade<disjoint<OuterIter>
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
            cursor_type(accum_iterator ait) : accum(ait), index(0) {}

            accum_iterator accum; // -> {Naccum, inner range}
            size_t index; // corresponding global index comparable to size()
            inner_iterator inner; // current inner iterator in range

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

        disjoint()
            : cursor(accums.end())
        {
            // cursor.accum = accums.end();
            // cursor.index = 0;       // equal to size
            // inners_size_ = 0;
        }

        // Construct a full iterator.
        disjoint(OuterIter beg, OuterIter end)
            : cursor(accums.end())
        {

            // fixme: for now, give up on figuring out how to
            // dynamically iterate just given the outer beg/end and
            // make a map that accumulates the inner ranges

            inners_size_ = 0;
            for (auto it = beg; it != end; ++it) {
                inner_range_type ir{it};
                if (ir.empty()) { continue; }
                accums[inners_size_] = ir;
                inners_size_ += ir.size();
            }

            cursor.index = 0;
            cursor.accum = accums.begin();
            if (inners_size_ > 0) {
                cursor.inner = cursor.accum->second.begin();
            } // undefined if we are empty.
        }

        void append(OuterIter it) {
            inner_range_type ir{it};
            if (ir.empty()) { return; }
            accums[inners_size_] = ir;
            inners_size_ += ir.size();
        }

        // Give range API
        disjoint begin() const {
            return *this;
        }
        disjoint end() const {
            auto it = *this;
            it.cursor.accum == it.accums.end();
            it.cursor.index = it.inners_size_;
            return it;
        }

        // Size of all inners
        size_t size() const { return inners_size_; }

      private:

        friend class boost::iterator_core_access;

        bool at_outer_end() const {
            return cursor.index == inners_size_; //  cursor.accum == accums.end();
        }

        bool at_outer_begin() const {
            return cursor.index == 0;
            // return cursor.accum == accums.begin()
            //     and cursor.inner == cursor.accum->second.begin();
        }

        template <typename OtherOuterIter>
        bool equal(disjoint<OtherOuterIter> const& x) const {
            if (at_outer_end() and x.at_outer_end()) {
                return true;
            }
            if (at_outer_end() or x.at_outer_end()) {
                return false;
            }
            if (cursor.index != x.cursor.index) {
                return false;
            }
            if (size() != x.size()) {
                return false;
            }
            if (accums.size() != x.accums.size()) {
                return false;
            }
            // what is left is to figure out if we both see the same set of inners.
            auto it1 = accums.begin();
            auto it2 = x.accums.begin();
            while (it1 != accums.end() and it2 != x.accums.end()) {
                if (it1->first == it2->first
                    and it1->second.outer == it2->second.outer) {
                    ++it1;
                    ++it2;
                    continue;
                }
                return false;
            }
            return true;        // hi me
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
            if (cursor.index == 0) {
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

        difference_type local_distance() const {
            return std::distance(cursor.accum->second.begin(), cursor.inner);
        }

        // Move to start of next outer
        void outer_increment() {
            ++cursor.accum;
            cursor.index = cursor.accum->first;
            cursor.inner = cursor.accum->second.begin();
        }

        // Move to start of previous outer
        void outer_decrement() {
            --cursor.accum;
            cursor.index = cursor.accum->first;
            cursor.inner = cursor.accum->second.begin();
        }

        void advance(difference_type n)
        {
            // local distance from current inner the bound of current outer.
            difference_type ldist = 0;

            // We are too high
            ldist = local_distance();
            while (n < ldist) {
                outer_decrement();
                n += ldist + cursor.accum->second.size();
                ldist = 0;
            }

            // We are too low
            ldist = cursor.accum->second.size() - ldist;
            while (n >= ldist) {
                n -= ldist;
                outer_increment();
                ldist = cursor.accum->second.size();
            }

            // just right
            cursor.index += n;
            cursor.inner += n;

            // Fixme: this method can be optimized by using the
            // cursor.accum.first and cursor.accum.size() to walk
            // outer itterators until landing on the one holding the
            // target distance.
            // if (n>0) {
            //     while (n) {
            //         ++(*this);
            //         --n;
            //     }
            //     return;
            // }
            // if (n<0) {
            //     while (n) {
            //         --(*this);
            //         ++n;
            //     }
            //     return;
            // }
        }

        difference_type distance_to(disjoint<OuterIter> const& other) const
        {
            return other.cursor.index - this->cursor.index;
        }

        reference dereference() const
        {
            return *cursor.inner;
        }

    };                              // disjoint

    // deduce types from arguments
    template <typename OuterIter>
    auto flatten(OuterIter begin, OuterIter end) -> disjoint<OuterIter>
    {
        return disjoint<OuterIter>(begin, end);
    }        
    template <typename OuterContainer>
    auto flatten(OuterContainer& cont) -> disjoint<typename OuterContainer::iterator>
    {
        return flatten(cont.begin(), cont.end());
    }

}

#endif
