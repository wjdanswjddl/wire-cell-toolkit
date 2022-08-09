#include "WireCellAux/BlobShadow.h"
#include "WireCellAux/ClusterHelpers.h"
#include "WireCellIface/ICluster.h"
#include "WireCellUtil/GraphTools.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>

using namespace WireCell;
using namespace WireCell::GraphTools;
using namespace WireCell::Aux;
using namespace WireCell::Aux::BlobShadow;



// A BFS visitor that records vertices of the given code.
template<typename Gr>
struct LeafVisitor : public boost::default_bfs_visitor
{
    using graph_t = Gr;
    using vertex_t = typename boost::graph_traits<Gr>::vertex_descriptor;
    using edge_t = typename boost::graph_traits<Gr>::edge_descriptor;
    
    char code;
    mutable std::vector<vertex_t> leaves;

    void examine_vertex(vertex_t v, const graph_t& g) const {
        if (g[v].code() == code) {
            leaves.push_back(v);
        }
    }
};


BlobShadow::graph_t BlobShadow::shadow(const cluster_graph_t& cgraph, char leaf_code)
{
    BlobShadow::graph_t bsgraph; // will return

    // Convert to directed so we do not "crawl up" from leafs
    auto dgraph = cluster::directed::type_directed(cgraph);
    LeafVisitor<cluster::directed::graph_t> leafvis{{}, leaf_code};

    // Loop over blobs, to load up output graph, old->new map.
    std::unordered_map<cluster::directed::vertex_t, BlobShadow::vdesc_t> c2bs;
    for (auto bvtx : mir(boost::vertices(dgraph))) {
        if (dgraph[bvtx].code() == 'b') {
            c2bs[bvtx] = boost::add_vertex({bvtx}, bsgraph);
        }
    }

    // Loop again, to pick up slices and make edges
    for (auto svtx : mir(boost::vertices(dgraph))) {
        if (dgraph[svtx].code() != 's') {
            continue;
        }

        // Keep track of every blob in a slice from which we came to a leaf.
        std::unordered_map<cluster::directed::vertex_t, std::vector<cluster::directed::vertex_t>> leaf2blob;

        // Loop over slice neighbor blobs.
        for (auto bedge : mir(boost::out_edges(svtx, dgraph))) {
            auto bvtx = boost::target(bedge, dgraph);

            leafvis.leaves.clear();
            boost::breadth_first_search(dgraph, bvtx, boost::visitor(leafvis));

            for (auto lvtx : leafvis.leaves) {
                leaf2blob[lvtx].push_back(bvtx);
            }
        }

        // Process leaves.
        for (const auto& [lvtx, bvtxs] : leaf2blob) {
            size_t nblobs = bvtxs.size();

            if (nblobs < 2) {
                continue;
            }

            // pair-wise blobs
            for (size_t ind1=0; ind1 < nblobs-1; ++ind1) {
                auto bvtx1 = c2bs[bvtxs[ind1]];

                for (size_t ind2=ind1+1; ind2 < nblobs; ++ind2) {
                    auto bvtx2 = c2bs[bvtxs[ind2]];

                    auto [edge, added] = boost::add_edge(bvtx1, bvtx2, bsgraph);

                    WirePlaneId wpid(0);
                    int index{-1};
                    const auto& obj = dgraph[lvtx];
                    const char lcode = obj.code();
                    if (lcode == 'w') { // wire
                        auto iptr = get<IWire::pointer>(obj.ptr);
                        wpid = iptr->planeid();
                        index = iptr->index();
                    }
                    else if (lcode == 'c') { // channel
                        auto iptr = get<IChannel::pointer>(obj.ptr);
                        wpid = iptr->planeid();
                        index = iptr->index();
                    }
                    else {
                        continue;
                    }
                    Edge& eobj = bsgraph[edge];
                    eobj.wpid = wpid;
                    if (added) {    // first time
                        eobj.beg = index;
                        eobj.end = index+1;
                    }
                    else {
                        eobj.beg = std::min(eobj.beg, index);
                        eobj.end = std::max(eobj.end, index+1);
                    }
                }
            }
        }
    }
    return bsgraph;
}

