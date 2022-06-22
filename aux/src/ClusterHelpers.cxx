// Very general helpers.
// See also ClusterHelper*.cxx

#include "WireCellAux/ClusterHelpers.h"

#include "WireCellIface/ISlice.h"
#include "WireCellIface/IFrame.h"

using namespace WireCell;

using slice_t = cluster_node_t::slice_t;



// maybe useful to export
ISlice::vector WireCell::Aux::find_slices(const ICluster& cluster)
{
    ISlice::vector ret;
    const auto& gr = cluster.graph();
    for (auto vtx : boost::make_iterator_range(boost::vertices(gr))) {
        const auto& vobj = gr[vtx];
        if (vobj.ptr.index() != 4) {
            continue;
        }

        auto islice = std::get<slice_t>(vobj.ptr);
        ret.push_back(islice);
    }
    return ret;
}

IFrame::pointer WireCell::Aux::find_frame(const ICluster& cluster)
{
    const auto& gr = cluster.graph();
    for (auto vtx : boost::make_iterator_range(boost::vertices(gr))) {
        const auto& vobj = gr[vtx];
        if (vobj.ptr.index() != 4) {
            continue;
        }

        auto islice = std::get<slice_t>(vobj.ptr);
        return islice->frame();
    }
    return nullptr;
}

std::string WireCell::Aux::name(const ICluster& cluster)
{
    std::stringstream ss;
    ss << "cluster_" << cluster.ident();
    return ss.str();
}
