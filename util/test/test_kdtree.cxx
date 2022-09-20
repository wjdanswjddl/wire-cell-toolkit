#include "WireCellUtil/KDTree.h"

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

    name_list_t nl = {"one", "two"};
    auto kdq = query_double(d, nl);
    std::vector<double> qp{2.0, 2.0};
    auto a = kdq->knn(2, qp);
    const size_t nfound = a.index.size();
    for (size_t ifound=0; ifound<nfound; ++ifound) {
        size_t ind = a.index[ifound];
        double dist = a.distance[ifound];
        std::cerr << ifound << ": [" <<ind<< "] p=("<<one[ind]<<","<<two[ind]<<") d=" << dist << "\n";
    }

}


int main() {
    test_static();

    return 0;
}
