#include "WireCellAux/RandTools.h"
#include "WireCellUtil/Math.h"

using namespace WireCell;
using namespace WireCell::Aux::RandTools::Normals;


Recycling::Recycling(IRandom::pointer rng,
                     size_t capacity,
                     double replacement_fraction,
                     double mean, double sigma)
    : rng(rng), normf(rng->make_normal(mean, sigma)), repfrac(replacement_fraction)
{
    resize(capacity);
}

void Recycling::resize(size_t capacity)
{
    const size_t oldsize = ring.size();
    ring.resize(capacity, 0);
    if (capacity > oldsize) {
        for (size_t ind=oldsize; ind<capacity; ++ind) {
            ring[ind] = normf();
        }
    }
    size_t jump = (1-repfrac)*capacity;
    jump = std::max(jump, 1UL);
    jump = std::min(jump, capacity-1);
    nreplace = replace = WireCell::nearest_coprime(capacity, jump);
}

// Return a pseudo-pseudo-random normal.
// This is only method that advances the cursor!
// We avoid modulus ("%") for speed (1.6x speedup)
float Recycling::operator()()
{
    const size_t size = ring.size();

    if (cursor == replace) {
        ring[cursor] = normf();
        replace = (replace + nreplace);
        while (replace >= size) {
            replace -= size;
        }
    }
    const float ret = ring[cursor];
    ++cursor;
    if (cursor == size) {       // ring
        cursor = 0;
    }
    return ret;
}

// Return a vector of pseudo-pseudo-random normals of size.
real_vector_t Recycling::operator()(size_t size)
{
    // Relying totally on coprime cycling leads to overly strong
    // correlations.  This also softens the need to be overly
    // concerned about ring capacity vs sampling size.  Though, still
    // best to to pick them coprime.
    cursor = rng->range(0, ring.size()-1);

    real_vector_t ret(size, 0);
    for (size_t ind=0; ind<size; ++ind) {
        ret[ind] = (*this)();
    }

    return ret;
}



Fresh::Fresh(IRandom::pointer rng,
             double mean, double sigma)
    : normf(rng->make_normal(mean, sigma))
{
}

// Return a pseudo-random normal
float Fresh::operator()() {
    return normf();
}

real_vector_t Fresh::operator()(size_t size)
{
    real_vector_t ret(size, 0);
    for (auto& f : ret) {
        f = normf();
    }
    return ret;
}
