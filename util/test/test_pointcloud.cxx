#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/Testing.h"

#include <vector>

using namespace WireCell;
using namespace WireCell::PointCloud;

void test_array()
{
    std::vector<int> v{1,2,3};
    const std::vector<int>& w = v;

    // cross: (mutable, const) x (shared, copy)
    Array ms(v.data(), {v.size()}, true);
    Array mc(v.data(), {v.size()}, false);
    Array cs(w.data(), {w.size()}, true);
    Array cc(w.data(), {w.size()}, false);

    Assert(ms.num_elements() == 3);
    Assert(mc.num_elements() == 3);
    Assert(cs.num_elements() == 3);
    Assert(cc.num_elements() == 3);

    v.push_back(4);
    ms.assign(v.data(), {v.size()}, true);
    mc.assign(v.data(), {v.size()}, false);
    cs.assign(w.data(), {w.size()}, true);
    cc.assign(w.data(), {w.size()}, false);

    Assert(ms.num_elements() == 4);
    Assert(mc.num_elements() == 4);
    Assert(cs.num_elements() == 4);
    Assert(cc.num_elements() == 4);

    v[0] = 42;

    Assert(ms.element<int>(0) == 42);
    Assert(mc.element<int>(0) == 1);
    Assert(cs.element<int>(0) == 42);
    Assert(cc.element<int>(0) == 1);

    std::vector<double> d{1., 2., 3.};
    const std::vector<double>& p=d;

    ms.assign(d.data(), {d.size()}, true);
    mc.assign(d.data(), {d.size()}, false);
    cs.assign(p.data(), {p.size()}, true);
    cc.assign(p.data(), {p.size()}, false);

    Assert(ms.num_elements() == 3);
    Assert(mc.num_elements() == 3);
    Assert(cs.num_elements() == 3);
    Assert(cc.num_elements() == 3);

    d.push_back(4);
    ms.assign(d.data(), {d.size()}, true);
    mc.assign(d.data(), {d.size()}, false);
    cs.assign(p.data(), {p.size()}, true);
    cc.assign(p.data(), {p.size()}, false);

    Assert(ms.num_elements() == 4);
    Assert(mc.num_elements() == 4);
    Assert(cs.num_elements() == 4);
    Assert(cc.num_elements() == 4);

    d[0] = 42;

    Assert(ms.element<double>(0) == 42);
    Assert(mc.element<double>(0) == 1);
    Assert(cs.element<double>(0) == 42);
    Assert(cc.element<double>(0) == 1);

    auto msarr = ms.indexed<double, 1>();
    Assert(msarr[0] == 42);

    auto msele = ms.elements<double>();
    Assert(msele[0] == 42);

    std::vector<double> d2{4.,5.,6.};
    ms.append(d2);
    Assert(ms.element<double>(3) == 4);

    ms.clear();
    mc.clear();
    cs.clear();
    cc.clear();

    Assert(ms.num_elements() == 0);
    Assert(mc.num_elements() == 0);
    Assert(cs.num_elements() == 0);
    Assert(cc.num_elements() == 0);

}

void test_dataset()
{
    Dataset::store_t s = {
        {"one", Array({1  ,2  ,3  })},
        {"two", Array({1.1,2.2,3.3})},
    };
    Dataset d(s);
    Assert(d.num_elements() == 3);
    Assert(d.keys().size() == 2);

    {
        Dataset d1(d);
        Assert(d.num_elements() == 3);
        Assert(d1.num_elements() == 3);
        Assert(d1.keys().size() == 2);
        
        Dataset d2 = d;
        Assert(d.num_elements() == 3);
        Assert(d2.num_elements() == 3);
        Assert(d2.keys().size() == 2);
    
        Dataset d3 = std::move(d2);
        Assert(d2.num_elements() == 0);
        Assert(d3.num_elements() == 3);
        Assert(d2.keys().size() == 0);
        Assert(d3.keys().size() == 2);
    }
    Assert(d.num_elements() == 3);
    
}

int main()
{
    test_array();
    test_dataset();

    return 0;
}
