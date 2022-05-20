#include "WireCellAux/RandTools.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/TimeKeeper.h"
#include "WireCellUtil/String.h"
#include "WireCellIface/IRandom.h"
#include "WireCellIface/IConfigurable.h"

#include <iostream>

using namespace WireCell;
using namespace WireCell::String;

void test_rn(IRandom::pointer rng)
{
    const size_t capacity = 10; // tiny
    Aux::RecyclingNormals rn(rng, capacity); 
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
    
    std::vector<size_t> sample_count{1000, 10000, 100000, 1000000};
    std::vector<size_t> capacities{100, 1000};


    for (auto nsamples : sample_count) {
        for (auto capacity : capacities) {
            Aux::RecyclingNormals rn(rng, capacity); 
            for (size_t ind=0; ind<nsamples; ++ind) {
                rn();
            }
            tk(format("RN: %d capacity=%d", nsamples, capacity));
            for (size_t ind=0; ind<nsamples; ++ind) {
                rn();
            }
            tk(format("RN: %d capacity=%d (again)", nsamples, capacity));
        }
        {
            Aux::FreshNormals fn(rng);
            for (size_t ind=0; ind<nsamples; ++ind) {
                fn();
            }
            tk(format("FN: %d", nsamples));
        }
    }
    std::cerr << tk.summary() << std::endl;

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

    test_speed(rng);

    return 0;
}
