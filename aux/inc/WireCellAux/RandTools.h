/** Some tools to do things with randoms.
 */

#ifndef WIRECELL_AUX_RANDTOOLS
#define WIRECELL_AUX_RANDTOOLS

#include "WireCellIface/IRandom.h"

#include <vector>

namespace WireCell::Aux {

    // A generator of normal Gaussian-distributed numbers which
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
    class RecyclingNormals {
      public:

        using real_vector_t = std::vector<float>;

        RecyclingNormals(IRandom::pointer rng,
                         size_t capacity = 1024,
                         double replacement_fraction=0.02);

        // Return a pseudo-pseudo-random normal
        float operator()();

        // Return a vector of pseudo-pseudo-random normals of size.
        real_vector_t operator()(size_t size);

        // The replacement period.
        size_t replacement() const { return replace; }

      private:
        IRandom::pointer rng;
        IRandom::double_func normf;
        size_t nreplace, replace;
        size_t cursor{0};  // may eventually roll over, we do not care
        real_vector_t ring;
    };

    // Return freshly generated normals. 
    class FreshNormals {
      public:
        using real_vector_t = std::vector<float>;

        FreshNormals(IRandom::pointer rng);

        // Return a pseudo-random normal
        float operator()();

        // Return a vector of pseudo-random normals of size.
        real_vector_t operator()(size_t size);


      private:
        IRandom::double_func normf;
    };

}

#endif
