#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/Testing.h"

#include <vector>
#include <iostream>

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

    {
        Assert(ms.shape().size() == 1);
        Assert(ms.shape()[0] == 3);
    }

    {
        auto flat_span = ms.elements<int>();
        Assert(flat_span.size() == v.size());
        ms.elements<float>(); // same size okay (for now)
        bool caught = true;
        try {
            ms.elements<double>();
        }
        catch (const ValueError& err) {
            caught = true;
        }
        Assert(caught);
    }

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
    {
        Assert(ms.shape().size() == 1);
        Assert(ms.shape()[0] == 7);
    }

    ms.clear();
    mc.clear();
    cs.clear();
    cc.clear();

    Assert(ms.num_elements() == 0);
    Assert(mc.num_elements() == 0);
    Assert(cs.num_elements() == 0);
    Assert(cc.num_elements() == 0);

}

void test_array2d()
{
    Array::shape_t shape = {2,5};
    std::vector<int> user(shape[0]*shape[1],0);
    Array arr(user, shape, true);
    Assert(arr.shape() == shape);
    arr.append(user);
    shape[0] += shape[0];
    Assert(arr.shape() == shape);    

    std::vector<double> fuser(10,0);
    bool caught = false;
    try {
        arr.append(fuser);      // should throw
        std::cerr << "failed to through on type size error\n";
    }
    catch (const ValueError& err) {
        std::cerr << "error on type mismatch as expected\n";
        caught=true;
    }
    Assert(caught);
    
    Assert(arr.shape() == shape);    

    {
        auto ma = arr.indexed<int, 2>();
        Assert(ma.num_dimensions() == 2);
        Assert(ma.num_elements() == 20);
        Assert(ma.shape()[0] == 4);
        Assert(ma.shape()[1] == 5);
    }

    {                           // wrong ndims
        bool caught=false;
        try {
            auto ma = arr.indexed<int, 3>();
        }
        catch (const ValueError& err) {
            caught = true;
        }
        Assert(caught);
    }

    {                           // wrong element size
        bool caught=false;
        try {
            auto ma = arr.indexed<double, 2>();
        }
        catch (const ValueError& err) {
            caught = true;
        }
        Assert(caught);
    }


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
    Assert(d.store().size() == 2);

    {
        auto d2 = d;
        Array::shape_t shape = {3,4};
        std::vector<int> user(shape[0]*shape[1],0);
        Array arr(user, shape, true);
        d2.add("twod", arr);
    }
    {
        auto d2 = d;
        Array::shape_t shape = {2,5};
        std::vector<int> user(shape[0]*shape[1],0);
        Array arr(user, shape, true);
        bool caught = false;
        try {
            d2.add("twod", arr);
        }
        catch (const ValueError& err) {
            caught = true;
        }
        Assert(caught);
    }

    {
        auto sel = d.selection({"one", "two"});
        Assert(sel.size() == 2);
        const Array& arr = sel[0];
        Assert(arr.num_elements() == 3);
        Assert(sel[1].get().num_elements() == 3);
    }

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

    {
        Dataset::store_t s = {
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})},
        };
        Dataset d(s);
        Assert(d.num_elements() == 3);
        Assert(d.keys().size() == 2);

        size_t beg=0, end=0;
        d.register_append([&](size_t b, size_t e) { beg=b; end=e; });

        Dataset tail;
        tail.add("one", Array({4  , 5}));
        Assert(tail.num_elements() == 2);

        bool caught = false;
        try {
            tail.add("two", Array({4.4}));
        }
        catch (const ValueError& err) {
            caught = true;
        }
        Assert(caught);

        tail.add("two", Array({4.4, 5.4}));
        Assert(tail.keys().size() == 2);
        Assert(tail.num_elements() == 2);

        d.append(tail);
        std::cerr << "HAS " << d.num_elements() << "\n";

        Assert(beg == 3);
        Assert(end == 5);

        Assert(d.keys().size() == 2);
        Assert(d.num_elements() == 5);

    }        

}

int main()
{
    test_array();
    test_array2d();
    test_dataset();

    return 0;
}
