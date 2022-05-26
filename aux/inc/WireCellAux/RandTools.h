/** Some tools to do things with randoms.
 */

#ifndef WIRECELL_AUX_RANDTOOLS
#define WIRECELL_AUX_RANDTOOLS

#include "WireCellIface/IRandom.h"

#include <vector>

namespace WireCell::Aux::RandTools::Normals {

    using real_vector_t = std::vector<float>;

    // A generator of Gaussian-distributed numbers which partly
    // "recycles" its prior results to trade off randomness for speed.
    // 
    // It generates a "pseudo-pseudo-random" stream by holding the
    // prior randoms in a fixed-capacity ring buffer which it samples
    // monotonically and it adds fresh randoms at a rate give by the
    // "replacement_fraction" parameter.
    //
    // It provides scalar and vector sampling operators.
    //
    // When the vector operator is use for constant size vectors it is
    // recomended to set the capacity to be coprime with the vector
    // size.  This will maximize the number of calls before a sampling
    // returns to a prior ring index.  See nearest_coprime() in
    // WireCellUtil/Math.h. for an easy way to provide that.
    //
    // With a 4% replacement, speedup is about 2x compared to fully
    // regenerating as in Fresh.  See test_noise.
    class Recycling {
      public:

        Recycling(IRandom::pointer rng, size_t capacity, 
                  double replacement_fraction=0.04,
                  double mean=0.0, double sigma=1.0);

        // Return a pseudo-pseudo-random normal
        float operator()();

        // Return a vector of pseudo-pseudo-random normals of size.
        real_vector_t operator()(size_t size);

        // Resize the ring.  Enlarged capcity is filled with fresh
        // randoms.
        void resize(size_t capacity);

        size_t size() const { return ring.size(); }

        // The replacement period.
        size_t replacement() const { return replace; }

      private:
        IRandom::pointer rng;
        IRandom::double_func normf;
        size_t nreplace, replace;
        double repfrac{0.02};
        size_t cursor{0};  // may eventually roll over, we do not care
        real_vector_t ring;
    };

    // Return freshly generated normals. 
    class Fresh {
      public:
        using real_vector_t = std::vector<float>;

        Fresh(IRandom::pointer rng,
              double mean=0.0, double sigma=1.0);

        // Return a pseudo-random normal
        float operator()();

        // Return a vector of pseudo-random normals of size.
        real_vector_t operator()(size_t size);


      private:
        IRandom::double_func normf;
    };

}

#endif
