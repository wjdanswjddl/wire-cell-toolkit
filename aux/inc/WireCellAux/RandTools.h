/** Some tools to do things with randoms.
 */

#ifndef WIRECELL_AUX_RANDTOOLS
#define WIRECELL_AUX_RANDTOOLS

#include "WireCellIface/IRandom.h"

#include <vector>

namespace WireCell::Aux::RandTools {

    using real_vector_t = std::vector<float>;

    // A generator that returns fresh randoms.
    using generator_f = std::function<double()>;

    // A base for pseudo RNGs
    struct PRNG {
        virtual ~PRNG() {}
        virtual float operator()() = 0;
        virtual real_vector_t operator()(size_t size) = 0;
    };

    class Recycling : virtual public PRNG {
      public:

        // Construct a "pseudo-pseudo" random number generator
        // producing number from approximately the same distribution
        // as produced by the given fresh pseudo random number
        // generator "gen".  A second fresh number generator of U(0,1)
        // distribution is also needed.  The recylcing is performed in
        // a ring buffer of size capacity and the replacement_fraction
        // determins how often fresh gen randoms are added.
        //
        // The gen and uni generators may be easily provided with:
        // 
        // IRandom::pointer rng = ...;
        // auto gen = rng->make_<distribution>(...);
        // auto uni = rng->make_uniform(0,1);
        //
        // When the vector operator is used to produce many vectors of
        // a fixed size it is recomended to set the capacity to be
        // coprime with the vector size.  This will maximize the
        // number of calls before a sampling returns to a prior ring
        // index.  See nearest_coprime() in WireCellUtil/Math.h. for
        // an easy way to provide that.
        //
        // With a 4% replacement, speedup is about 2x compared to
        // fully regenerating as in Fresh.  See test_noise.
        Recycling(generator_f gen, generator_f uni,
                  size_t capacity=1000,
                  double replacement_fraction=0.04);

        // Return a single pseudo-pseudo-random 
        float operator()();

        // Return a vector of pseudo-pseudo-random uniform of size.
        real_vector_t operator()(size_t size);

        // Resize the ring.  Enlarged capcity is filled with fresh
        // randoms.
        void resize(size_t capacity);

        size_t size() const { return ring.size(); }

        // The replacement period.
        size_t replacement() const { return replace; }

      private:
        generator_f gen, uni;
        size_t nreplace, replace;
        double repfrac{0.02};
        size_t cursor{0};  // may eventually roll over, we do not care
        real_vector_t ring;
    };

    // Return all freshly generated randoms in an API matching
    // Recycling.
    class Fresh : virtual public PRNG {
      public:

        Fresh(generator_f gen);
        float operator()();
        real_vector_t operator()(size_t size);

      private:
        generator_f gen;
    };

    
    // Helpers to make above

    namespace Normals {
        inline
        RandTools::Recycling
        make_recycling(IRandom::pointer rng, size_t capacity, 
                       double mean=0.0, double sigma=1.0,
                       double replacement_fraction=0.04)
        {
            return RandTools::Recycling(rng->make_normal(mean, sigma),
                                        rng->make_uniform(0,1),
                                        capacity, replacement_fraction);
        }
        inline
        RandTools::Fresh
        make_fresh(IRandom::pointer rng,
                   double mean=0.0, double sigma=1.0)
        {
            return RandTools::Fresh(rng->make_normal(mean, sigma));
        }
    }

    namespace Uniforms {
        inline
        RandTools::Recycling
        make_recycling(IRandom::pointer rng, size_t capacity, 
                       double lo=0.0, double hi=1.0,
                       double replacement_fraction=0.04)
        {
            return RandTools::Recycling(rng->make_uniform(lo, hi),
                                        rng->make_uniform(0,1),
                                        capacity, replacement_fraction);
        }

        inline
        RandTools::Fresh
        make_fresh(IRandom::pointer rng, double lo=0.0, double hi=1.0)
        {
            return RandTools::Fresh(rng->make_uniform(lo, hi));
        }
    }

}

#endif
