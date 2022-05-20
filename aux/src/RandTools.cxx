#include "WireCellAux/RandTools.h"
#include "WireCellUtil/Math.h"

using namespace WireCell;


Aux::RecyclingNormals::RecyclingNormals(IRandom::pointer rng,
                                        size_t capacity,
                                        double replacement_fraction)
    : rng(rng), ring(capacity, 0)
{
    const size_t r = std::min(capacity-1, (size_t)((1-replacement_fraction)*capacity));
    nreplace = replace = WireCell::nearest_coprime(capacity, r);
    // start fully fresh
    for (size_t ind=0; ind<capacity; ++ind) {
        ring[ind] = rng->normal(0,1);
    }
}

float& Aux::RecyclingNormals::at(size_t index)
{
    return ring[index%ring.size()];
}

// Return a pseudo-pseudo-random normal
float Aux::RecyclingNormals::operator()()
{
    const size_t ind = cursor % ring.size();
    if (cursor == replace) {
        ring[ind] = rng->normal(0,1);
        replace += nreplace;
    }
    float ret = ring[ind];
    ++cursor;
    return ret;
}

// Return a vector of pseudo-pseudo-random normals of size.
Aux::RecyclingNormals::real_vector_t Aux::RecyclingNormals::operator()(size_t size)
{
    // Don't use std::generate as it does a copy of *this.
    Aux::RecyclingNormals::real_vector_t ret(size, 0);
    for (auto& f : ret) {
        f = (*this)();
    }
    return ret;
}



Aux::FreshNormals::FreshNormals(IRandom::pointer rng) : rng(rng) {}
// Return a pseudo-random normal
float Aux::FreshNormals::operator()() { return rng->normal(0,1); }

Aux::FreshNormals::real_vector_t Aux::FreshNormals::operator()(size_t size)
{
    Aux::RecyclingNormals::real_vector_t ret(size, 0);
    for (auto& f : ret) {
        f = (*this)();
    }
    return ret;
}
