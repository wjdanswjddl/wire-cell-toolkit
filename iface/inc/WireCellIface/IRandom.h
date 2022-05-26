/** IRandom - interface to random number generators.

    WCT code should be given an IRandom instead of directly using any
    lower level random number services. 

    The IRandom API provides generation of values from various
    distributions.  Each distribution has a pair of methods:

    The "immediate" methods directly return a random value and the
    "callable" methods return function object that when called will
    return a random value.

    When a distribution with particular parameters must be sampled
    repeatedly doing so through a "callable" may be faster than using
    the corresponding "immediate" method.

    Note, to gain any speed up, the IRandom implementation must
    explicitly implement these "callable" methods.  

 */

#ifndef WIRECELL_IRANDOM
#define WIRECELL_IRANDOM

#include "WireCellUtil/IComponent.h"
#include <functional>

namespace WireCell {

    class IRandom : public IComponent<IRandom> {
       public:
        virtual ~IRandom();

        using int_func = std::function<int()>;
        using double_func = std::function<double()>;

        /// Sample a binomial distribution
        virtual int binomial(int max, double prob) = 0;
        virtual int_func make_binomial(int max, double prob);

        /// Sample a Poisson distribution.
        virtual int poisson(double mean) = 0;
        virtual int_func make_poisson(double mean);

        /// Sample a normal distribution.
        virtual double normal(double mean, double sigma) = 0;
        virtual double_func make_normal(double mean, double sigma);

        /// Sample a uniform distribution
        virtual double uniform(double begin, double end) = 0;
        virtual double_func make_uniform(double begin, double end);

        /// Sample an exponential distribution
        virtual double exponential(double mean) = 0;
        virtual double_func make_exponential(double mean);

        /// Sample a uniform integer range.
        virtual int range(int first, int last) = 0;
        virtual int_func make_range(int first, int last);
    };

}  // namespace WireCell

#endif
