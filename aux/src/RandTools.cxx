#include "WireCellAux/RandTools.h"
#include "WireCellUtil/Math.h"

using namespace WireCell;


Aux::RecyclingNormals::RecyclingNormals(IRandom::pointer rng,
                                        size_t capacity,
                                        double replacement_fraction)
    : rng(rng), normf(rng->make_normal(0,1)), ring(capacity, 0)
{
    size_t jump = (1-replacement_fraction)*capacity;
    jump = std::max(jump, 1UL);
    jump = std::min(jump, capacity-1);
    nreplace = replace = WireCell::nearest_coprime(capacity, jump);
    // start fully fresh
    for (size_t ind=0; ind<capacity; ++ind) {
        ring[ind] = normf();
    }
}

// Return a pseudo-pseudo-random normal.
// This is only method that advances the cursor!
// We avoid modulus ("%") for speed (1.6x speedup)
float Aux::RecyclingNormals::operator()()
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
Aux::RecyclingNormals::real_vector_t Aux::RecyclingNormals::operator()(size_t size)
{
    // Relying totally on coprime cycling leads to overly strong
    // correlations.  This also softens the need to be overly
    // concerned about ring capacity vs sampling size.  Though, still
    // best to to pick them coprime.
    cursor = rng->range(0, ring.size()-1);

    Aux::RecyclingNormals::real_vector_t ret(size, 0);
    for (size_t ind=0; ind<size; ++ind) {
        ret[ind] = (*this)();
    }

    return ret;
}



Aux::FreshNormals::FreshNormals(IRandom::pointer rng)
    : normf(rng->make_normal(0,1))
{
}

// Return a pseudo-random normal
float Aux::FreshNormals::operator()() {
    return normf();
}

Aux::FreshNormals::real_vector_t Aux::FreshNormals::operator()(size_t size)
{
    Aux::RecyclingNormals::real_vector_t ret(size, 0);
    for (auto& f : ret) {
        f = normf();
    }
    return ret;
}
