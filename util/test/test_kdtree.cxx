#include "WireCellUtil/KDTree.h"
#include "WireCellUtil/Testing.h"

#include <vector>
#include <string>
#include <iostream>


using namespace WireCell::PointCloud;
using namespace WireCell::KDTree;

void test_static()
{
    Dataset::store_t s = {
        {"one", Array({1.0,1.0,3.0})},
        {"two", Array({1.1,2.2,3.3})},
    };
    Dataset d(s);

    auto one = s["one"].elements<double>();
    auto two = s["two"].elements<double>();

    auto kdq = query_double(d, {"one", "two"});
    {
        auto a = kdq->knn(2, {2.0, 2.0});
        const size_t nfound = a.index.size();
        std::cerr << nfound << " found of 2 nearest neighbors\n";
        Assert(nfound == 2);
        for (size_t ifound=0; ifound<nfound; ++ifound) {
            size_t ind = a.index[ifound];
            double dist = a.distance[ifound];
            std::cerr << ifound << ": [" <<ind<< "] p=("<<one[ind]<<","<<two[ind]<<") d=" << dist << "\n";
        }
    }
    {
        // The extreme point is at linear r = 4.4598206.
        const double linear_radius = 4.46;
        // we use L2 metric!
        const double metric_radius = linear_radius*linear_radius; 

        auto a = kdq->radius(metric_radius, {0.0, 0.0});
        const size_t nfound = a.index.size();
        std::cerr << nfound << " found in radius\n";
        Assert(nfound == 3);
        for (size_t ifound=0; ifound<nfound; ++ifound) {
            size_t ind = a.index[ifound];
            double dist = a.distance[ifound];
            std::cerr << ifound << ": [" <<ind<< "] p=("<<one[ind]<<","<<two[ind]<<") d=" << dist << "\n";
        }
    }
    {
        // Catch just the first point
        const double linear_radius = 1.49;
        // we use L2 metric!
        const double metric_radius = linear_radius*linear_radius; 

        auto a = kdq->radius(metric_radius, {0.0, 0.0});
        const size_t nfound = a.index.size();
        std::cerr << nfound << " found in radius\n";
        Assert(nfound == 1);
        for (size_t ifound=0; ifound<nfound; ++ifound) {
            size_t ind = a.index[ifound];
            double dist = a.distance[ifound];
            std::cerr << ifound << ": [" <<ind<< "] p=("<<one[ind]<<","<<two[ind]<<") d=" << dist << "\n";
            Assert(ind == 0);
        }
    }
}

void test_dynamic()
{
    Dataset::store_t s = {
        {"one", Array({1.0,1.0,3.0})},
        {"two", Array({1.1,2.2,3.3})},
    };
    Dataset d(s);

    // This must have registered update callback.
    auto kdq = query_double(d, {"one", "two"}, true);

    auto arrs = d.selection({"one", "two"});

    const Array& one = arrs[0];
    const Array& two = arrs[1];

    Assert(one.num_elements() == 3);
    Assert(two.num_elements() == 3);

    Dataset tail({
            {"one", Array({1.1})},
            {"two", Array({1.0})}});
    d.append(tail);

    Assert(one.num_elements() == 4);
    Assert(two.num_elements() == 4);

    const double linear_radius = 1.49;
    const double metric_radius = linear_radius*linear_radius; 

    auto ones = one.elements<double>();
    auto twos = two.elements<double>();

    auto a = kdq->radius(metric_radius, {0.0, 0.0});
    const size_t nfound = a.index.size();
    std::cerr << nfound << " found in radius\n";
    Assert(nfound == 2);
    for (size_t ifound=0; ifound<nfound; ++ifound) {
        size_t ind = a.index[ifound];
        double dist = a.distance[ifound];
        std::cerr << ifound << ": [" <<ind<< "] p=("<<ones[ind]<<","<<twos[ind]<<") d=" << dist << "\n";
    }

}

int main() {
    test_static();
    test_dynamic();
    return 0;
}
