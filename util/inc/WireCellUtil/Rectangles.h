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
#include <tuple>
#include "boost/icl/interval_map.hpp"

namespace WireCell {

    /*
      Track rectangular "regions" (2D intervals).

      Key gives the type for rectangular coordinates.

      Value gives the type of data associated to regions.

      Set is used to collect Value objects and needs operator+= defined.

      Interval defines the interval type.  See boost::icl docs.

     */
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
        // An rectangle and a value.
        using rectangle_t = std::tuple<interval_t, interval_t, value_t>;
        // An region and a set of values.
        using region_t = std::tuple<interval_t, interval_t, set_t>;

        // add a half-open rectangle.  Eg, width from [x1,x2).
        void add(const key_t& x1, const key_t& x2,
                 const key_t& y1, const key_t& y2,
                 const value_t& val)
        {
            add(interval_t::right_open(x1,x2),
                interval_t::right_open(y1,y2), val);
        }

        void add(const interval_t xi, const interval_t& yi, const value_t& val) {
            ymap_t ymap;
            ymap += std::make_pair(yi, set_t{val});
            m_xmap += std::make_pair(xi, ymap);
        }

        // icl-like interface
        void operator+=(const rectangle_t& rec) {
            add(std::get<0>(rec), std::get<1>(rec), std::get<2>(rec));
        }
        
        const xmap_t& xmap() const { return m_xmap; }

        std::vector<region_t> regions() const 
        {
            std::vector<region_t> ret;
            for (const auto& [xi, ymap] : m_xmap) {
                for (const auto& [yi, cs] : ymap) {
                    ret.push_back(region_t(xi, yi, cs));
                }
            }
            return ret;
        }

      private:
        xmap_t m_xmap;
    };
}


#endif
