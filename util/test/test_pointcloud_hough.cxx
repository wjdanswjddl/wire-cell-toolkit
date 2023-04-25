#include "WireCellUtil/Point.h"
#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/KDTree.h"

#include <boost/histogram.hpp>

#include <random>
#include <vector>
#include <string>

using namespace WireCell;
using namespace WireCell::PointCloud;
namespace bh = boost::histogram;

int main()
{
    const double pi = 3.141592653589793;

    Dataset ds;

    Ray box(Point(-1, -2, -3), Point(1, 2, 3));
    const size_t npts = 1000;
    std::random_device rd;
    std::default_random_engine re(rd());
    name_list_t three = {"x", "y", "z"};

    for (size_t dim=0; dim<3; ++dim) {
        std::uniform_real_distribution<> dist(box.first[dim], box.second[dim]);
        std::vector<double> arr(npts);
        std::generate(arr.begin(), arr.end(), [&](){ return dist(re); });
        ds.add(three[dim], Array(arr));
    }
    {
        const double qmax = 10;
        std::uniform_real_distribution<> dist(0, qmax);
        std::vector<double> arr(npts);
        std::generate(arr.begin(), arr.end(), [&](){ return dist(re); });
        ds.add("q", Array(arr));
        
    }

    const size_t nnn = 10;
    auto query = KDTree::query<double>(ds, three);

    const auto& x = ds.get("x").elements<double>();
    const auto& y = ds.get("y").elements<double>();
    const auto& z = ds.get("z").elements<double>();
    const auto& q = ds.get("q").elements<double>();
        
    
    const Vector X(1,0,0);
    const Vector Y(0,1,0);
    const Vector Z(0,0,1);

    const double r2min = 0.01;
    size_t nhistbins = 100;
    auto hist = bh::make_histogram(bh::axis::regular<>( nhistbins, -1.0, 1.0 ),
                                   bh::axis::regular<>( nhistbins,  -pi, pi ) );
    for (size_t ipt=0; ipt<npts; ipt+=10) {
        hist.reset();

        const Point pt( x[ipt], y[ipt], z[ipt] );

        auto resnn = query->knn(nnn, { x[ipt], y[ipt], z[ipt] });
        const size_t nres = resnn.index.size();
        for (size_t ind=0; ind<nres; ++ind) {
            const double r2 = resnn.distance[ind];
            const size_t index = resnn.index[ind];
            const Point pti(x[index], y[index], z[index]);

            double w = q[index];
            if (r2 > r2min) {
                w *= r2min/r2;
            }

            Vector dir = (pti-pt).norm();
            const double cth = Z.dot(dir);
            const double phi = atan2(Y.dot(dir), X.dot(dir));

            hist(cth, phi, bh::weight(w));
        }
        {
            auto indexed = bh::indexed(hist);
            auto it = std::max_element(indexed.begin(), indexed.end());
            const auto& cell = *it;
            std::cerr << ipt 
                      << " maximum: index=[" << cell.index(0) <<","<< cell.index(1) <<"]"
                      << " cth:[" << cell.bin(0).lower() << "," << cell.bin(0).upper() << "]"
                      << " phi:[" << cell.bin(1).lower() << "," << cell.bin(1).upper() << "]"
                      << " value=" << *cell
                      << " pt=" << pt
                      << "\n";
        }
        
    }
    return 0;
}

