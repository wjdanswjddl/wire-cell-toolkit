#ifndef WIRECELL_AUX_RAND
#define WIRECELL_AUX_RAND

namespace WireCell::Aux {

    // A generator of normal Gaussian-distributed numbers that
    // "recycles" its results to trade off randomness for speed.  It
    // maintains a fixed-capacity ring buffer that it samples such
    // that the stream returned by each call begins at a random
    // location in the ring.  The buffer is filled with new randomness
    // at a rate give by the "replace" parameter which is used to find
    // an integer which coprime with the capacty an near to
    // int(replace*capacity) modulo capacity.
    class RecyclingNormals {
        IRandom::pointer rng
        size_t replace;
        size_t cursor{0};
        real_vector_t ring;
      public:

        /// 
        RecyclingNormals(IRandom::pointer rng,
                         double replacment_fraction=0.02,
                         size_t capacity = 1024)
            : rng(rng), ring(capacity, 0)
        {
            replace = WireCell::nearest_coprime(capacity, (1-replacement_fraction)*capacity);
            // start fully fresh
            for (size_t ind=0; ind<capacity; ++ind) {
                ring[ind] = rng->normal(0,1);
            }
        }

        float& at(size_t index) {
            return ring[index%ring.size()];
        }

        float next() {
            const size_t ind = cursor % ring.size();

            if (cursor % replace == 0) {
                ring[ind] = rng->normal(0,1);
            }
            float ret = ring[ind];
            ++cursor;
            return ret;
        }

    }

}

#endif
