
#include "WireCellImg/GeomClusteringUtil.h"

#include "WireCellUtil/String.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/RayClustering.h"

using namespace WireCell;
using namespace WireCell::Img;

// return the time between two blob sets relative to the slice span of
// the first.
static int rel_time_diff(const IBlobSet::pointer& one, const IBlobSet::pointer& two)
{
    const auto here = one->slice();
    return std::round((two->slice()->start() - here->start()) / here->span());
}

void WireCell::Img::geom_clustering(cluster_indexed_graph_t& grind, IBlobSet::vector::iterator beg,
                                                  IBlobSet::vector::iterator end, std::string policy)
{
    std::cout << "geom_clustering: " << std::endl;
    auto next = beg + 1;

    if (next == end) {
        return;
    }

    std::unordered_set<std::string> known_policies = {"simple", "uboone", "uboone_local"};
    if (known_policies.find(policy) == known_policies.end()) {
        THROW(ValueError() << errmsg{String::format("policy \"%s\" not implemented!", policy)});
    }

    // uboone policy
    int max_rel_diff = 2;
    std::map<int, RayGrid::grid_index_t> map_gap_tol = {{1, 2}, {2, 1}};

    if (policy == "uboone_local") {
        max_rel_diff = 2;
        map_gap_tol = {{1, 1}, {2, 1}};
    }

    if (policy == "simple") {
        max_rel_diff = 1;
        map_gap_tol = {{1, 0}};
    }

    // overlap + tolerance
    struct TolerantVisitor {
        RayGrid::grid_index_t tolerance{0};
        bool verbose{false};
        RayGrid::blobvec_t operator()(const RayGrid::blobref_t& blob, const RayGrid::blobproj_t& proj,
                                      RayGrid::layer_index_t layer)
        {
            return RayGrid::overlap(blob, proj, layer, tolerance, verbose);
        }
    };

    for (auto test = next; test != end and rel_time_diff(*beg, *test) <= max_rel_diff; ++test) {
        int rel_diff = rel_time_diff(*beg, *test);
        // TODO: remove debug info
        // int btick = (*beg)->slice()->start()/((*beg)->slice()->span())*4;
        // int ttick = (*test)->slice()->start()/((*test)->slice()->span())*4;
        // if(btick == 664 && ttick == 668) {
        //     std::cout << "btick: " << btick << ", ttick: " << ttick << std::endl;
        // }
        if (map_gap_tol.find(rel_diff) == map_gap_tol.end()) continue;

        // handle each face separately faces
        IBlob::vector iblobs1 = (*test)->blobs();
        IBlob::vector iblobs2 = (*beg)->blobs();

        RayGrid::blobs_t blobs1 = (*test)->shapes();
        RayGrid::blobs_t blobs2 = (*beg)->shapes();
        // TODO: remove debug info
        // if(btick == 664 && ttick == 668) {
        //     std::cout << "tolerence: " << map_gap_tol[rel_diff] << std::endl;
        //     std::cout << "beg:\n";
        //     for (size_t i=0; i< iblobs2.size(); ++i) {
        //         std::cout << iblobs2[i].get()->ident() << std::endl;
        //         std::cout << blobs2[i].as_string() << std::endl;
        //     }
        //     std::cout << "test:\n";
        //     for (size_t i=0; i< iblobs1.size(); ++i) {
        //         std::cout << iblobs1[i].get()->ident() << std::endl;
        //         std::cout << blobs1[i].as_string() << std::endl;
        //     }
        // }

        const auto beg1 = blobs1.begin();
        const auto beg2 = blobs2.begin();

        auto assoc = [&](RayGrid::blobref_t& a, RayGrid::blobref_t& b) {
            int an = a - beg1;
            int bn = b - beg2;
            grind.edge(iblobs1[an], iblobs2[bn]);
            // TODO: remove debug info
            // if(btick == 664 && ttick == 668) {
            //     std::cout << iblobs1[an].get()->ident() << " --- " << iblobs2[bn].get()->ident() << std::endl;
            // }
        };
        bool verbose = false;
        // TODO: remove debug info
        // if (btick == 664 && ttick == 668) {
        //     verbose = true;
        // }
        TolerantVisitor tv{map_gap_tol[rel_diff], verbose};
        RayGrid::associate(blobs1, blobs2, assoc, tv);
    }
}