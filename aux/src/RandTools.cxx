#include "WireCellAux/RandTools.h"
#include "WireCellUtil/Math.h"

using namespace WireCell;
using namespace WireCell::Aux::RandTools;


Recycling::Recycling(generator_f gen, generator_f uni,
                     size_t capacity, double replacement_fraction)
    : gen(gen), uni(uni), repfrac(replacement_fraction)
{
    resize(capacity);
}

void Recycling::resize(size_t capacity)
{
    const size_t oldsize = ring.size();
    ring.resize(capacity, 0);
    if (capacity > oldsize) {
        for (size_t ind=oldsize; ind<capacity; ++ind) {
            ring[ind] = gen();
        }
    }
    size_t jump = 1/repfrac;
    jump = std::max(jump, 1UL);
    jump = std::min(jump, capacity-1);
    nreplace = replace = WireCell::nearest_coprime(capacity, jump);
    // nreplace = replace = jump;
}

// This is only method that advances the cursor!
// We avoid modulus ("%") for speed (1.6x speedup)
float Recycling::operator()()
{
    const size_t size = ring.size();

    if (cursor == replace) {
        ring[cursor] = gen();
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

real_vector_t Recycling::operator()(size_t size)
{
    // Relying totally on coprime cycling leads to overly strong
    // correlations.  This also softens the need to be overly
    // concerned about ring capacity vs sampling size.  Though, still
    // best to to pick them coprime.
    cursor = uni() * ring.size()-1;
    cursor = cursor % ring.size(); // in case uni is bigger than [0,1]

    // A random start point loses "sync" with the replacement fraction
    // and too many operator()() calls can be made until the cursor
    // may catch up to the "replace" cursor.  At the cost of an extra
    // random draw, set replace cursor to here.  As a side effect,
    // this guarantees the first sample is freshly random.
    replace = cursor;

    real_vector_t ret(size, 0);
    for (size_t ind=0; ind<size; ++ind) {
        ret[ind] = (*this)();
    }

    return ret;
}



Fresh::Fresh(generator_f  gen)
    : gen(gen)
{
}

float Fresh::operator()() {
    return gen();
}

real_vector_t Fresh::operator()(size_t size)
{
    real_vector_t ret(size, 0);
    std::generate(ret.begin(), ret.end(), gen);
    return ret;
}
