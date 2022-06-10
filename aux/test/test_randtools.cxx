#include "WireCellAux/RandTools.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/TimeKeeper.h"
#include "WireCellUtil/String.h"
#include "WireCellIface/IConfigurable.h"

#include <iostream>

using namespace WireCell;
using namespace WireCell::String;
using namespace WireCell::Aux::RandTools;

void test_rn(IRandom::pointer rng)
{
    const size_t capacity = 10; // tiny
    Recycling rn(rng->make_normal(0,1), rng->make_uniform(0,1), capacity); 
    for (size_t ind=0; ind<100; ++ind) {
        std::cerr << ind << ": " << rn() << "\n";
    }
    for (size_t ind=0; ind<400; ++ind) {
        // Note, this a bad choice for capcity and vector size.  With
        // size == capacity/10 the 1st draw and 11'th draw will return
        // the same location of the ring buffer and we will only see
        // freshness due to refresh fraction.
        auto got = rn(10);      
        if (ind%10 == 0) {
            for (auto x : got) {
                std::cerr << x << " ";
            }
            std::cerr << "\n";
        }
    }
    std::cerr << "capacity=" << capacity << " replacement=" << rn.replacement() << "\n";
}

void test_speed(IRandom::pointer rng)
{
    TimeKeeper tk("random normals");
    
    std::vector<size_t> sample_count{1000000, 10000000};
    std::vector<size_t> capacities{100, 1000};


    for (auto nsamples : sample_count) {
        for (auto capacity : capacities) {
            Recycling rn(rng->make_normal(0,1), rng->make_uniform(0,1), capacity); 
            for (size_t ind=0; ind<nsamples; ++ind) {
                rn();
            }
            tk(format("RN: %d capacity=%d", nsamples, capacity));
            for (size_t ind=0; ind<nsamples; ++ind) {
                rn();
            }
            tk(format("RN: %d capacity=%d (again)", nsamples, capacity));
        }
        for (auto capacity : capacities) {
            Recycling ru(rng->make_uniform(0,1), rng->make_uniform(0,1), capacity); 
            for (size_t ind=0; ind<nsamples; ++ind) {
                ru();
            }
            tk(format("RU: %d capacity=%d", nsamples, capacity));
            for (size_t ind=0; ind<nsamples; ++ind) {
                ru();
            }
            tk(format("RU: %d capacity=%d (again)", nsamples, capacity));
        }
        {
            Fresh fn(rng->make_normal(0,1));
            for (size_t ind=0; ind<nsamples; ++ind) {
                fn();
            }
            tk(format("FN: %d", nsamples));
        }
        {
            Fresh fn(rng->make_uniform(0,1));
            for (size_t ind=0; ind<nsamples; ++ind) {
                fn();
            }
            tk(format("FU: %d", nsamples));
        }
        {
            auto u = rng->make_uniform(0,1);
            for (size_t ind=0; ind<nsamples; ++ind) {
                u();
            }
            tk(format("IU: %d", nsamples));
        }
    }

    std::cerr << tk.summary() << std::endl;

}

void test_freshness(IRandom::pointer rng)
{
    Recycling rn(rng->make_normal(0,1), rng->make_uniform(0,1), 10); 
    auto one = rn(100);
    auto two = rn(100);
    for (size_t ind=0; ind<100; ++ind) {
        std::cerr << ind << "\t" << one[ind] << "\t" << two[ind] << "\n";
    }
}

int main()
{
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellGen");
    {
        auto rngcfg = Factory::lookup<IConfigurable>("Random");
        auto cfg = rngcfg->default_configuration();
        rngcfg->configure(cfg);
    }
    auto rng = Factory::lookup<IRandom>("Random");

    test_rn(rng);

    test_freshness(rng);

    test_speed(rng);

    return 0;
}
