/*
  This provides a 2D version of an interval tree.

  For concepts see:

  https://en.wikipedia.org/wiki/Interval_tree

  It is based on Boost's ICL which provides 1D interval trees:

  https://www.boost.org/doc/libs/1_80_0/libs/icl/doc/html/index.html

  In short, it lets you do this:

                                              
  ┌──────┐                  ┌──────┐
  │ a    │                  └──────┘a
  │  ┌───┼───┐              ┌─┐┌───┐┌──┐
  │  │   │   │   disjoin    │a││a,b││b │
  └──┼───┘   │  ─────────>  └─┘└───┘└──┘
     │ b     │                 ┌───────┐
     └───────┘                 └───────┘b


  Thanks to herbstluftwm(1) for the diagram.

 */

#ifndef WIRECELL_UTIL_RECTANGLES
#define WIRECELL_UTIL_RECTANGLES

#include <set>
#include "boost/icl/interval_map.hpp"

namespace WireCell {

    /*
      Track rectangular "regions" (2D intervals).

      Key gives the type for rectangular coordinates.

      Value gives the type of data associated to regions.

      Set is used to collect Value objects and needs operator+= defined.

      Interval defines the interval type.  See boost::icl docs.

     */
    // fixme: could be extended to N dimensions with template<int N>
    // and lots of variadic templating.
    template<typename Key,
             typename Value,
             typename Set = std::set<Value>,
             typename Interval = typename boost::icl::interval<Key>::interval_type>
    class Rectangles {
      public:
        using key_t = Key;
        using value_t = Value;
        using set_t = Set;
        using interval_t = Interval;

        // Intervals along the vertical Y-axis
        using ymap_t = boost::icl::interval_map<key_t, set_t>;
        // Intervals along the horizontal X-axis
        using xmap_t = boost::icl::interval_map<key_t, ymap_t>;
        // A rectangle is a pair of intervals: (xi,yi).
        using rectangle_t = std::pair<interval_t, interval_t>;
        // An element in the map is a pair of rectangle and value
        using element_t = std::pair<rectangle_t, value_t>;
        // A region is a contiguous rectangle and the set of values
        using region_t = std::pair<rectangle_t, set_t>;

        // Add a half-open rectangle.  Eg, width from [x1,x2), etc height.
        void add(const key_t& x1, const key_t& x2,
                 const key_t& y1, const key_t& y2,
                 const value_t& val)
        {
            add(interval_t::right_open(x1,x2),
                interval_t::right_open(y1,y2), val);
        }

        // Add rectangle as pair of intervals.
        void add(const interval_t xi, const interval_t& yi, const value_t& val) {
            ymap_t ymap;
            ymap += std::make_pair(yi, set_t{val});
            m_xmap += std::make_pair(xi, ymap);
        }

        // Add rectangle as rectangle.
        void add(const rectangle_t& r, const value_t& val) {
            add(r.first, r.second, val);
        }

        // icl-like interface
        void operator+=(const element_t& ele) {
            add(ele.first, ele.second);
        }
        
        // Access the top interval map along X-axis.
        const xmap_t& xmap() const { return m_xmap; }

        // Return all contiguous regions of the overlapping
        // rectangles.
        std::vector<region_t> regions() const 
        {
            std::vector<region_t> ret;
            for (const auto& [xi, ymap] : m_xmap) {
                for (const auto& [yi, cs] : ymap) {
                    ret.push_back(region_t(rectangle_t(xi, yi), cs));
                }
            }
            return ret;
        }

        // Return regions that are the intersectection stored
        // rectangles and the given rectangle.
        std::vector<region_t> intersection(const rectangle_t& rec) const
        {
            return intersection(rec.first, rec.second);
        }
        // As above but for x/y intervals.
        std::vector<region_t> intersection(const interval_t& xi, const interval_t& yi) const
        {
            std::vector<region_t> ret;
            for (const auto& [qxi, ymap] : m_xmap & xi) {
                for (const auto& [qyi, qs] : ymap & yi) {
                    ret.push_back(region_t(rectangle_t(qxi, qyi), qs));
                }
            }
            return ret;
        }

      private:
        xmap_t m_xmap;
    };
}


#endif
