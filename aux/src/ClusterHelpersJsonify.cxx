#include "WireCellAux/ClusterHelpers.h"

#include "WireCellIface/IChannel.h"
#include "WireCellIface/IAnodeFace.h"
#include "WireCellIface/IWire.h"
#include "WireCellIface/IBlob.h"
#include "WireCellIface/ISlice.h"
#include "WireCellIface/IFrame.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/String.h"

using namespace WireCell;

using channel_t = cluster_node_t::channel_t;
using wire_t = cluster_node_t::wire_t;
using blob_t = cluster_node_t::blob_t;
using slice_t = cluster_node_t::slice_t;
using meas_t = cluster_node_t::meas_t;


static Json::Value size_stringer(const cluster_node_t& n) { return Json::nullValue; }

static Json::Value jpoint(const Point& p)
{
    Json::Value ret;
    for (int ind = 0; ind < 3; ++ind) {
        ret[ind] = p[ind];
    }
    return ret;
}

static Json::Value channel_jsoner(const cluster_node_t& n)
{
    auto ich = std::get<channel_t>(n.ptr);
    Json::Value ret = Json::objectValue;
    ret["ident"] = ich->ident();
    ret["index"] = ich->index();
    ret["wpid"] = ich->planeid().ident();
    return ret;
}

static Json::Value wire_jsoner(const cluster_node_t& n)
{
    auto iwire = std::get<wire_t>(n.ptr);
    Json::Value ret = Json::objectValue;
    ret["ident"] = iwire->ident();
    ret["index"] = iwire->index();
    ret["wpid"] = iwire->planeid().ident();
    ret["chid"] = iwire->channel();
    ret["seg"] = iwire->segment();
    auto r = iwire->ray();
    ret["tail"] = jpoint(r.first);
    ret["head"] = jpoint(r.second);
    return ret;
}

// this one needs extra info
Json::Value blob_jsoner(const cluster_node_t& n)
{
    auto iblob = std::get<blob_t>(n.ptr);
    IAnodeFace::pointer iface = iblob->face();
    IWirePlane::vector iplanes = iface->planes();
    auto iwire = iplanes[2]->wires()[0];

    auto islice = iblob->slice();

    // set "X" as the slice start time.  Note, this line represents a
    // breaking change but one which I think is for the best.  We used
    // to bake-in a drift speed in order to convert t0 to x0.  This
    // can now fall onto the responsibility of any consumers of the
    // data, if needed.  Better to keep the data clear from such
    // arbitrary transformation.
    double t0 = islice->start();

    const auto& coords = iface->raygrid();

    Json::Value ret = Json::objectValue;
    ret["span"] = islice->span();
    ret["start"] = islice->start();
    ret["ident"] = iblob->ident();
    ret["val"] = iblob->value();
    ret["unc"] = iblob->uncertainty();
    {
        WirePlaneId afid(kUnknownLayer, iface->which(), iface->anode());
        ret["faceid"] = afid.ident();
    }
    ret["sliceid"] = iblob->slice()->ident();
    Json::Value jcorners = Json::arrayValue;
    const auto& blob = iblob->shape();
    for (const auto& c : blob.corners()) {
        Json::Value j = jpoint(coords.ray_crossing(c.first, c.second));
        j[0] = t0;
        jcorners.append(j);
    }
    ret["corners"] = jcorners;

    Json::Value jbounds = Json::arrayValue;
    for (const auto& strip : blob.strips()) {
        int plane_ind = strip.layer - 2;
        if (plane_ind <0) {
            continue;           // skip active region bounds
        }
        Json ::Value j = Json::objectValue;
        j["beg"] = strip.bounds.first;
        j["end"] = strip.bounds.second;
        jbounds.append(j);
    }
    ret["bounds"] = jbounds;

    return ret;
}

static
Json::Value slice_jsoner(const cluster_node_t& n)
{
    auto islice = std::get<slice_t>(n.ptr);
    Json::Value ret = Json::objectValue;

    ret["ident"] = islice->ident();
    ret["frameid"] = islice->frame()->ident();
    ret["start"] = islice->start();
    ret["span"] = islice->span();

    // note: used to be SOA, now AOS.
    Json::Value jsignal = Json::arrayValue;
    for (const auto& it : islice->activity()) {
        Json::Value jact = Json::objectValue;
        jact["ident"] = it.first->ident();
        const double val = it.second.value();
        jact["val"] = val;
        jact["unc"] = it.second.uncertainty();
        jsignal.append(jact);
    }
    ret["signal"] = jsignal;
    return ret;
}

static
Json::Value measurement_jsoner(const cluster_node_t& n)
{
    auto imeas = std::get<meas_t>(n.ptr);
    auto sig = imeas->signal();
    Json::Value ret = Json::objectValue;
    ret["ident"] = imeas->ident();
    ret["val"] = sig.value();
    ret["unc"] = sig.uncertainty();
    ret["wpid"] = imeas->planeid().ident();
    // note: used to have chids, which is redundant with the graph
    return ret;
}

Json::Value WireCell::Aux::jsonify(const cluster_graph_t& gr)
{
    std::vector<std::function<Json::Value(const cluster_node_t& ptr)> > jsoners{
        size_stringer, channel_jsoner, wire_jsoner, blob_jsoner, slice_jsoner, measurement_jsoner};
    auto asjson = [&](const cluster_node_t& n) { return jsoners[n.ptr.index()](n); };

    /*
     * - nodes :: a list of graph nodes
     * - edges :: a list of graph edges
     *
     * A node is represented as a JSON object with the following attributes
     * - ident :: an indexable ID number for the node, and referred to in "edges"
     * - type :: the letter "code" used in ICluster: one in "sbcwm"
     * - data :: an object holding information about the corresponding vertex object
     *
     * An edge is a pair of vertex ident numbers.
     *
     */

    Json::Value jnodes = Json::arrayValue;
    for (auto vtx : boost::make_iterator_range(boost::vertices(gr))) {
        const auto& vobj = gr[vtx];
        if (!vobj.ptr.index()) {
            // warn?
            continue;
        }
        Json::Value jnode = Json::objectValue;
        jnode["ident"] = (int) vtx;
        jnode["type"] = String::format("%c", vobj.code());
        jnode["data"] = asjson(vobj);

        jnodes.append(jnode);
    }

    Json::Value jedges = Json::arrayValue;
    for (auto eit : boost::make_iterator_range(boost::edges(gr))) {
        Json::Value jedge = Json::objectValue;
        jedge["tail"] = (int) boost::source(eit, gr);
        jedge["head"] = (int) boost::target(eit, gr);
        jedges.append(jedge);
    }

    Json::Value top = Json::objectValue;
    top["nodes"] = jnodes;
    top["edges"] = jedges;

    return top;
}
