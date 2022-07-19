#include "WireCellUtil/Binning.h"
#include "WireCellUtil/Testing.h"

#include <vector>
#include <iostream>

using namespace WireCell;

void test_subset()
{
    Binning a(10, 0, 10);
    std::cerr << "a: " << a << "\n";
    {
        auto s = subset(a, 0.5, 10);
        std::cerr << "s1: " << s << "\n";
        Assert(s.nbins()   == a.nbins());
        Assert(s.binsize() == a.binsize());
        Assert(s.min()     == a.min());
        Assert(s.max()     == a.max());
    }
    {
        auto s = subset(a, 1, 10);
        std::cerr << "s2: " << s << "\n";
        Assert(s.nbins()   == a.nbins() - 1);
        Assert(s.binsize() == a.binsize());
        Assert(s.min()     == a.min() + 1);
        Assert(s.max()     == a.max());
    }
    {
        auto s = subset(a, -1, 10);
        std::cerr << "s3: " << s << "\n";
        Assert(s.nbins()   == a.nbins());
        Assert(s.binsize() == a.binsize());
        Assert(s.min()     == a.min());
        Assert(s.max()     == a.max());
    }
    {
        Binning b(122, 1.528e+06, 1.772e+06);
        std::cerr << "b: " << b << "\n";
        auto s = subset(b, 1791637.8683138976, 1795821.8405835067);
        std::cerr << "s: " << s << "\n";
        Assert(s.nbins() == 0);
        
    }
}

void test_gaussian()
{
    Binning a(10,0,10);
    std::cerr << "a: " << a << "\n";
    const double mean = 3.0, sigma=1.0;
    std::cerr << "mean="<< mean << " sigma=" << sigma << "\n";
    std::vector<double> g(a.nbins(), 0);
    const double total = gaussian(g.begin(), a, 3.0, 1.0);
    double sum = 0;
    for (int ind=0; ind<10; ++ind) {
        const double got = g[ind];
        sum += got;
        std::cerr << ind <<": [center=" << a.center(ind) << "] = " << got << "\n";
    }
    const double diff = total-sum;
    std::cerr << "total=" << total << " sum=" << sum << " diff=" << diff << "\n";
    Assert(std::abs(diff) < 1e-6);
}
int main()
{
    test_subset();
    test_gaussian();
    return 0;
}
