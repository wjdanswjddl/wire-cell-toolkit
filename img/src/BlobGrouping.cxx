#include "WireCellImg/BlobGrouping.h"

#include "WireCellIface/SimpleCluster.h"
#include "WireCellIface/SimpleMeasure.h"

#include "WireCellUtil/NamedFactory.h"

#include <boost/graph/connected_components.hpp>

WIRECELL_FACTORY(BlobGrouping, WireCell::Img::BlobGrouping,
                 WireCell::INamed,
                 WireCell::IClusterFilter, WireCell::IConfigurable)

using namespace WireCell;

typedef std::unordered_map<WirePlaneLayer_t, cluster_indexed_graph_t> layer_graphs_t;

Img::BlobGrouping::BlobGrouping()
    : Aux::Logger("BlobGrouping", "img")
{
}

Img::BlobGrouping::~BlobGrouping() {}

void Img::BlobGrouping::configure(const WireCell::Configuration& cfg) {}

WireCell::Configuration Img::BlobGrouping::default_configuration() const
{
    WireCell::Configuration cfg;
    return cfg;
}

static void fill_blob(layer_graphs_t& lgs, const cluster_indexed_graph_t& grind, IBlob::pointer iblob)
{
    cluster_node_t nblob{iblob};

    for (auto wvtx : grind.neighbors(nblob)) {
        if (wvtx.code() != 'w') {
            continue;
        }
        auto iwire = std::get<IWire::pointer>(wvtx.ptr);
        auto layer = iwire->planeid().layer();
        auto& lg = lgs[layer];

        for (auto cvtx : grind.neighbors(wvtx)) {
            if (cvtx.code() != 'c') {
                continue;
            }
            lg.edge(nblob, cvtx);
        }
    }
}

void Img::BlobGrouping::fill_slice(cluster_indexed_graph_t& grind, ISlice::pointer islice)
{
    layer_graphs_t lgs;

    auto activity = islice->activity();

    for (auto other : grind.neighbors(islice)) {
        if (other.code() != 'b') {
            continue;
        }
        IBlob::pointer iblob = std::get<IBlob::pointer>(other.ptr);
        fill_blob(lgs, grind, iblob);
    }

    // measures
    for (auto lgit : lgs) {
        auto& lgrind = lgit.second;
        auto groups = lgrind.groups();
        for (auto& group : groups) {

            // Really, the data held by a "measure" is redundant with
            // its location in the graph and the data held by the
            // connected IChannels and the ISlice found via the
            // connected IBlobs.
            SimpleMeasure* smeas = new SimpleMeasure{m_mcount++};
            IMeasure::pointer imeas(smeas);

            for (auto& v : group.second) {
                if (v.code() == 'b') {
                    // (b-m)
                    grind.edge(v.ptr, imeas);
                    continue;
                }
                if (v.code() == 'c') {
                    // (c-m)
                    smeas->sig += activity[std::get<IChannel::pointer>(v.ptr)];
                    grind.edge(v.ptr, imeas);
                    continue;
                }
            }
        }
    }
}

bool Img::BlobGrouping::operator()(const input_pointer& in, output_pointer& out)
{
    if (!in) {
        log->debug("EOS");
        out = nullptr;
        return true;
    }

    cluster_indexed_graph_t grind(in->graph());

    m_mcount = 0;

    for (auto islice : oftype<ISlice::pointer>(grind)) {
        fill_slice(grind, islice);
    }

    log->debug("cluster {}: nvertices={} nedges={} nmeas={}",
               in->ident(),
               boost::num_vertices(grind.graph()),
               boost::num_edges(grind.graph()),
               m_mcount);

    out = std::make_shared<SimpleCluster>(grind.graph(), in->ident());
    return true;
}
