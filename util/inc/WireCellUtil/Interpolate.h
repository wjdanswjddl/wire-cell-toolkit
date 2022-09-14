/**
   Interpolationn helpers.

   - irrterp :: interpolate on irregular samples (note: slow for point interpolation, but okay for sequence)

   - linterp :: interpolate on regular samples (fast for point or sequence).

   Example usage:

   // For either
   using X = float;
   using Y = float;

   X xstart = 0.0;
   X xstep = 1.0;

   // for irrterp
   using collection_t = std::map<X,Y>;
   collection_t col = ...;
   irrterp terp(col.begin(), col.end());

   // for linterp
   using collection_t = std::vector<Y>;
   collection_t col = ...;
   X x0=0, xstep=42;
   linterp terp(col.begin(), col.end(), x0, xstep);

   // for either - point interpolation
   X x = 6.9;
   Y y = terp(x);

   // for either - sequence interpolation
   std::vector<Y> ys;
   size_t num=10;
   X xstart=0, xstep=0.1;
   terp(std::back_inserter(ys), num, xstart, xstep);
   cerr << "filled " << std::distance(ys.begin(), end) << "\n";

   See test_interpolate.cxx.
 */

#ifndef WIRECELLUTIL_INTERPOLATE
#define WIRECELLUTIL_INTERPOLATE

#include <map>
#include <cmath>
#include <vector>
#include <algorithm>
#include <stdexcept>
	
// c++20 has this defined in <cmath>.  For older language, here is the
// naive implementation that has some corner cases.
#if __cplusplus < 202000
namespace std {
    template<typename Real>
    Real lerp(Real a, Real b, Real t) {
        return a + t * (b - a);
    }
}
#endif

namespace WireCell {


    /**
       Linear interpolation on irregular sampling

       Note, this is logarithmic in the number of points for single
       point iteration.  

     */
    template <typename X, typename Y = X>
    class irrterp {
        std::map<X, Y> points;
      public:
        using xtype = X;
        using ytype = Y;

        template<typename PairIter>
        irrterp(PairIter beg, PairIter end) : points(beg, end) {}

        // This is logarithmic in #points.  Use sequence for faster
        // regular sampling.
        Y operator()(const X& x) {
            if (points.empty()) {
                throw std::logic_error("interpolation on empty sampling");
            }
            auto ub = points.upper_bound(x);
            if (ub == points.begin()) {
                return points.begin()->second;
            }
            if (ub == points.end()) {
                return points.rbegin()->second;
            }
            auto lb = ub;
            --lb;
            const X dX = ub->first - lb->first;
            const Y dY = ub->second - lb->second;
            const X dx = x - lb->first;
            return lb->second + dY * dx/dX;
        }

        // Insert num evenly spaced values between start and stop,
        // inclusive.
        template<typename OutputIterator>
        OutputIterator operator()(OutputIterator out, size_t num,
                                  const X& xstart, const X& xstep)
        {
            if (points.empty()) {
                throw std::logic_error("interpolation on empty sampling");
            }

            // dispense with special cases
            if (num == 0) {
                return out;
            }
            if (num == 1) {
                *out = (*this)(xstart);
                ++out;
                return out;
            }

            // Mark current X location
            X xcur = xstart;

            // We will assure loop always starts with ub above xcur.
            auto ub = points.upper_bound(xcur);
            while (num) {

                // we are before the domain
                if (ub == points.begin()) { 
                    *out = ub->second;
                    ++out;
                    --num;
                    xcur += xstep;
                    while (xcur > ub->first) {
                        ++ub;                        
                        if (ub == points.end()) {
                            break; // fell off domain
                        }
                    }
                    continue;
                }

                // we are after the domain
                if (ub == points.end()) { 
                    *out = points.rbegin()->second;
                    ++out; --num;
                    xcur += xstep;
                    continue;
                }

                // we are in the domain
                auto lb = ub;
                --lb;
                // casts X to Y
                Y delta = (xcur-lb->first) / (ub->first-lb->first);
                *out = std::lerp(lb->second, ub->second, delta);
                ++out; --num;
                xcur += xstep;
                while (xcur > ub->first) {
                    ++ub;
                    if (ub == points.end()) { 
                        break;  // fell off domain
                    } 
                }
            }
            return out;
        }
    };
    


    /**
       Use like:

          linterp<double> lin(f.begin(), f.end(), x0, dx);
          ...
          double y = lin(42.0);

       where "f" is some kind of collection of doubles.

     */
    template <typename X, typename Y = X>
    class linterp {
       public:
        using xtype = X;
        using ytype = Y;

        template <class Iterator>
        linterp(Iterator beg, Iterator end, X x0, X dx)
          : m_dat(beg, end)
          , m_le(x0)
          , m_step(dx)
        {
            m_re = m_le + m_step * (m_dat.size() - 1);
        }

        Y operator()(X x) const
        {
            if (x <= m_le) return m_dat.front();
            if (x >= m_re) return m_dat.back();

            int ind = int((x - m_le) / m_step);
            X x0 = m_le + ind * m_step;
            // casts X to Y
            Y delta = (x-x0)/m_step;
            return std::lerp(m_dat[ind], m_dat[ind+1], delta);
        }

        template<typename OutputIterator>
        OutputIterator operator()(OutputIterator out, size_t num,
                                  const X& xstart, const X& xstep)
        {
            // dispense with special cases
            if (num == 0) {
                return out;
            }
            if (num == 1) {
                *out = (*this)(xstart);
                ++out;
                return out;
            }

            for (size_t ind=0; ind<num; ++ind) {
                const X xcur = xstart + ind*xstep;
                *out = (*this)(xcur);
                ++out;
            }
            return out;
        }

       private:
        std::vector<Y> m_dat;
        X m_le, m_re, m_step;
    };

    // Simpler interface to one-shot linear interpolation 
    template<typename InputIt, typename OutputIt>
    void linterpolate(InputIt ibeg, InputIt iend, OutputIt obeg, OutputIt oend)
    {
        const double xmin = 0.0; // arbitrary but
        const double xmax = 1.0; // same for both.
        const size_t ilen = std::distance(ibeg, iend);
        const double olddx = (xmax-xmin)/ilen;
        linterp<double, typename InputIt::value_type> terp(ibeg, iend, xmin, olddx);
        const size_t olen = std::distance(obeg, oend);
        const double newdx = (xmax-xmin)/olen;
        terp(obeg, olen, xmin, newdx);
    }


    /** You may also want to use Boost for fancier interpolation.
     * They have similar calling interface:
     #include <boost/math/interpolators/cubic_b_spline.hpp>
     #include <iostream>
     ...
     boost::math::cubic_b_spline<double> spline(f.begin(), f.end(), x0, xstep);
     spline(42);
     *
     * More info here:
     * https://www.boost.org/doc/libs/1_78_0/libs/math/doc/html/interpolation.html
     */
    // for now, we do not force this as Boost gives internal
    // deprecation warnings at compile time.
    // template <class Real>
    // using cubterp = boost::math::cubic_b_spline<Real>;


}  // namespace WireCell

#endif
