#include "WireCellAux/ClusterHelpers.h"

#include <boost/graph/graphviz.hpp>

#include "WireCellUtil/String.h"

#include <fstream>
#include <sstream>
#include <functional>
#include <unordered_set>


using namespace WireCell;

static std::string asstring(const cluster_node_t& n)
{
    if (n.code() == '0') {
        size_t sp = std::get<size_t>(n.ptr);
        std::stringstream ss;
        ss << n.code() << ":" << sp;
        return ss.str();
    }
    
    std::stringstream ss;
    ss << n.code() << ":" << n.ident();
    return ss.str();
}

struct label_writer_t {
    const cluster_graph_t& g;
    template <class vdesc_t>
    void operator()(std::ostream& out, const vdesc_t v) const
    {
        out << "[label=\"" << asstring(g[v]) << "\"]";
    }
};

std::string Aux::dotify(const ICluster& cluster,
                        const std::string& codes)
{
    std::unordered_set<char> keep(codes.begin(), codes.end());

    // use indexed graph basically just for the copy()
    const cluster_graph_t& gr = cluster.graph();
    cluster_indexed_graph_t grind;

    for (const auto& v : boost::make_iterator_range(boost::vertices(gr))) {
        const auto& vobj = gr[v];
        if (keep.size() and keep.count(vobj.code()) == 0) {
            continue;
        }
        grind.vertex(vobj);
        for (auto edge : boost::make_iterator_range(boost::out_edges(v, gr))) {
            auto v2 = boost::target(edge, gr);
            const auto& vobj2 = gr[v2];
            if (keep.empty() or keep.count(vobj2.code())) {
                grind.edge(vobj, vobj2);
            }
        }
    }
    const cluster_graph_t& gr2 = grind.graph();
    std::stringstream out;
    boost::write_graphviz(out, gr2, label_writer_t{gr2});
    return out.str();
}
