//#include "WireCellUtil/Disjoint.h"
#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"

// local imp
#include <boost/iterator/iterator_adaptor.hpp>
#include <map>


using namespace WireCell;
using spdlog::debug;

template <typename OuterIter>
class disjoint_iterator :
    public boost::iterator_facade<disjoint_iterator<OuterIter>,
                                  // value
                                  typename OuterIter::value_type::iterator::value_type,
                                  // cagegory
                                  //boost::random_access_traversal_tag
                                  boost::forward_traversal_tag
                                  >
{
  private:
    using outer_iterator = OuterIter;
    using inner_iterator = typename OuterIter::value_type::iterator;

    struct inner_range_type {
        outer_iterator outer;
        // Number of inners in range
        size_t size() const { return std::distance(begin(), end()); }
        inner_iterator begin() const { return outer->begin(); }
        inner_iterator end() const { return outer->end(); }
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
    void trace(const std::string& msg) {
        debug("{}: index={}", msg, cursor.index);
    }

    // Construct an "end" iterator.
    disjoint_iterator(OuterIter end)
    {
        trace("construct(end)");
        cursor.accum = accums.end();
        cursor.index = 0;       // equal to size
        trace("constructed(end)");
    }

    // Construct a full iterator.
    disjoint_iterator(OuterIter beg, OuterIter end)
    {
        trace("construct(beg,end)");

        for (auto it = beg; it != end; ++it) {
            inner_range_type ir{it};
            if (ir.empty()) { continue; }
            accums[inners_size_] = ir;
            inners_size_ += ir.size();
        }

        if (inners_size_ == 0) {
            trace("construct(end,end)");
            cursor.accum = accums.end();
            cursor.index = 0;       // equal to size
            trace("constructed(end,end)");
            return;
        }

        // initialize cursor at start
        cursor.accum = accums.begin();
        cursor.inner = cursor.accum->second.begin();
        cursor.index = 0;
        trace("constructed(beg,end)");
    }

    // Size of all inners
    size_t size() const { return inners_size_; }

  private:

    friend class boost::iterator_core_access;

    bool at_outer_end() const {
        return cursor.accum == accums.end();
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
        trace("incrementing");
        if (at_outer_end()) {
            throw std::runtime_error("increment past end");
        }

        if (cursor.at_inner_end()) {
            // go next cursor
            ++cursor.accum;
            // fell off
            if (at_outer_end()) {
                cursor.inner = inner_iterator{};
                cursor.index = size();
                trace("increment at end");
                return;
            }
            // at start of non-empty range
            cursor.inner = cursor.accum->second.begin();
            ++cursor.index;
            trace("increment inter-range");
            return;
        }
        // in range
        ++cursor.inner;
        ++cursor.index;
        trace("increment intra-range");
    }

    // void advance(difference_type n)
    // {
        
    // }

    // difference_type distance_to(disjoint_iterator<OuterIter> const& other) const
    // {
    //     return cursor.index - this->cursor.index;
    // }


    using value_type = typename OuterIter::value_type::iterator::value_type;
    value_type& dereference() const
    {
        return *cursor.inner;
    }
};



template<typename OuterIter>
struct disjoint_range {
    using iterator = disjoint_iterator<OuterIter>;

    OuterIter b, e;
    iterator begin() const { return iterator(b,e); }
    iterator end() const { return iterator(e); }
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

TEST_CASE("disjoint iterator simple") {
    std::vector<std::vector<int>> hier;
    auto di1 = flatten(hier.begin(), hier.end());
    auto di2 = flatten(hier.end());
    CHECK(di1 == di2);
}

TEST_CASE("disjoint iterator no empties") {
    std::vector<std::vector<int>> hier = { {0,1,2}, {3}, {4} };

    for (auto i = flatten(hier.begin(), hier.end());
         i != flatten(hier.end());
         ++i) {
        debug("dji: {}", *i);
    }
}
TEST_CASE("disjoint iterator with empties") {
    std::vector<std::vector<int>> hier = { {}, {0,1,2}, {}, {3}, {4}, {}};

    for (auto i = flatten(hier.begin(), hier.end());
         i != flatten(hier.end());
         ++i) {
        debug("dji: {}", *i);
    }
}
