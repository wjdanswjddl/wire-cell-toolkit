#include "WireCellIface/IRandom.h"

#include <functional>

using namespace WireCell;

IRandom::~IRandom() {}

// We implement default closures around the abstract implementation.
// Concrete implementations (see Random) are encouraged to return
// closures around a generator in order that subsequent calls need not
// constantly construct it anew.

IRandom::int_func IRandom::make_binomial(int max, double prob)
{
    return std::bind(&IRandom::binomial, this, max, prob);
}

IRandom::int_func IRandom::make_poisson(double mean)
{
    return std::bind(&IRandom::poisson, this, mean);
}

IRandom::double_func IRandom::make_normal(double mean, double sigma)
{
    return std::bind(&IRandom::normal, this, mean, sigma);
}

IRandom::double_func IRandom::make_uniform(double begin, double end)
{
    return std::bind(&IRandom::uniform, this, begin, end);
}

IRandom::double_func IRandom::make_exponential(double mean)
{
    return std::bind(&IRandom::exponential, this, mean);
}
    
IRandom::int_func IRandom::make_range(int first, int last)
{
    return std::bind(&IRandom::range, this, first, last);
}

