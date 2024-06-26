/*
  Implementation notes:

  Implementation of Random uses actually another indirection
  (pointer-to-implementation pattern).  This is done in order to allow
  for use of different C++ std engines and because the standard does
  not deem it necessary to have these engines follow an inheritance
  hierarchy.

 */

#include "WireCellGen/Random.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"

#include <random>

WIRECELL_FACTORY(Random, WireCell::Gen::Random, WireCell::IRandom, WireCell::IConfigurable)

using namespace WireCell;
using spdlog::warn;

Gen::Random::Random(const std::string& generator, const std::vector<unsigned int> seeds)
  : m_generator(generator)
  , m_seeds(seeds.begin(), seeds.end())
  , m_pimpl(nullptr)
{
}

template<typename Number, typename URNG, typename Distro>
struct Closure {
    URNG& rng;
    Distro dist;
    template<typename T1>
    Closure(URNG& rng, const T1& arg1) : rng(rng), dist(arg1) {}
    template<typename T1, typename T2>
    Closure(URNG& rng, const T1& arg1, const T2& arg2) : rng(rng), dist(arg1, arg2) {}
    Number operator()() { return dist(rng); }
};


// This pimpl may turn out to be a bottle neck.
template <typename URNG>
class RandomT : public IRandom {
    URNG m_rng;

   public:
    RandomT(std::vector<unsigned int> seeds)
    {
        std::seed_seq seed(seeds.begin(), seeds.end());
        m_rng.seed(seed);
    }

    virtual int binomial(int max, double prob)
    {
        std::binomial_distribution<int> distribution(max, prob);
        return distribution(m_rng);
    }
    virtual int_func make_binomial(int max, double prob)
    {
        return Closure<int, URNG, std::binomial_distribution<int>>(m_rng, max,prob);
    }

    virtual int poisson(double mean)
    {
        std::poisson_distribution<int> distribution(mean);
        return distribution(m_rng);
    }
    virtual int_func make_poisson(double mean)
    {
        return Closure<int, URNG, std::poisson_distribution<int>>(m_rng, mean);
    }

    virtual double normal(double mean, double sigma)
    {
        std::normal_distribution<double> distribution(mean, sigma);
        return distribution(m_rng);
    }
    virtual double_func make_normal(double mean, double sigma)
    {
        return Closure<double, URNG, std::normal_distribution<double>>(m_rng, mean, sigma);
    }

    virtual double uniform(double begin, double end)
    {
        std::uniform_real_distribution<double> distribution(begin, end);
        return distribution(m_rng);
    }
    virtual double_func make_uniform(double begin, double end)
    {
        return Closure<double, URNG, std::uniform_real_distribution<double>>(m_rng, begin, end);
    }

    virtual double exponential(double mean)
    {
        std::exponential_distribution<double> distribution(mean);
        return distribution(m_rng);
    }
    virtual double_func make_exponential(double mean)
    {
        return Closure<double, URNG, std::exponential_distribution<double>>(m_rng, mean);
    }

    virtual int range(int first, int last)
    {
        std::uniform_int_distribution<int> distribution(first, last);
        return distribution(m_rng);
    }
    virtual int_func make_range(int first, int last)
    {
        return Closure<int, URNG, std::uniform_int_distribution<int>>(m_rng, first, last);
    }
};

void Gen::Random::configure(const WireCell::Configuration& cfg)
{
    auto jseeds = cfg["seeds"];
    if (not jseeds.isNull()) {
        std::vector<unsigned int> seeds;
        for (auto jseed : jseeds) {
            seeds.push_back(jseed.asInt());
        }
        m_seeds = seeds;
    }
    auto gen = get(cfg, "generator", m_generator);
    if (m_pimpl) {
        delete m_pimpl;
    }
    if (gen == "default") {
        m_pimpl = new RandomT<std::default_random_engine>(m_seeds);
    }
    else if (gen == "twister") {
        m_pimpl = new RandomT<std::mt19937>(m_seeds);
    }
    else if (gen == "") {
        m_pimpl = new RandomT<std::mt19937>(m_seeds);
    }
    else {
        warn("Gen::Random::configure: warning: unknown random engine: \"{}\" using default", gen);
        m_pimpl = new RandomT<std::default_random_engine>(m_seeds);
    }
}

WireCell::Configuration Gen::Random::default_configuration() const
{
    Configuration cfg;
    cfg["generator"] = m_generator;
    Json::Value jseeds(Json::arrayValue);
    for (auto seed : m_seeds) {
        jseeds.append(seed);
    }
    cfg["seeds"] = jseeds;
    return cfg;
}

int Gen::Random::binomial(int max, double prob) { return m_pimpl->binomial(max, prob); }
int Gen::Random::poisson(double mean) { return m_pimpl->poisson(mean); }
double Gen::Random::normal(double mean, double sigma) { return m_pimpl->normal(mean, sigma); }
double Gen::Random::uniform(double begin, double end) { return m_pimpl->uniform(begin, end); }
double Gen::Random::exponential(double mean) { return m_pimpl->exponential(mean); }
int Gen::Random::range(int first, int last) { return m_pimpl->range(first, last); }


IRandom::int_func Gen::Random::make_binomial(int max, double prob) { return m_pimpl->make_binomial(max, prob); }
IRandom::int_func Gen::Random::make_poisson(double mean) { return m_pimpl->make_poisson(mean); }
IRandom::double_func Gen::Random::make_normal(double mean, double sigma) { return m_pimpl->make_normal(mean, sigma); }
IRandom::double_func Gen::Random::make_uniform(double begin, double end) { return m_pimpl->make_uniform(begin, end); }
IRandom::double_func Gen::Random::make_exponential(double mean) { return m_pimpl->make_exponential(mean); }
IRandom::int_func Gen::Random::make_range(int first, int last) { return m_pimpl->make_range(first, last); }
