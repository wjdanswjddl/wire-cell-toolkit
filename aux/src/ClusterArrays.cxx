#include "WireCellAux/ClusterArrays.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/GraphTools.h"

#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/copy.hpp>


using namespace WireCell;
using namespace WireCell::Aux;
using namespace WireCell::GraphTools;

using channel_t = cluster_node_t::channel_t;
using wire_t = cluster_node_t::wire_t;
using blob_t = cluster_node_t::blob_t;
using slice_t = cluster_node_t::slice_t;
using meas_t = cluster_node_t::meas_t;

// Some hard-wired schema values
// See ClusterArray.org docs and keep in sync.
// For the common node array columns.
// Fixme: remove these when move to Dataset store
static const size_t desc_col=0; // all node and edge types
static const size_t ident_col=1; // all node types
static const size_t sigv_col=2;  // all but wires
static const size_t sigu_col=3;  // all but wires

// Edges do not have an integer descriptor, per se but we put an edge
// counter in the desc_col.
static const size_t tail_col=1;  // edge tail
static const size_t head_col=2;  // edge head


ClusterArrays::ClusterArrays()
{
}

ClusterArrays::~ClusterArrays()
{
}

ClusterArrays::ClusterArrays(const cluster_graph_t& graph)
{
    this->init(graph);
}

void ClusterArrays::clear()
{
    m_na.clear();
    m_ea.clear();
    m_v2s.clear();
}

template<typename ArrayType>
void rezero(ArrayType& arr, const std::vector<size_t>& sv)
{
    // resize and zero
    arr.resize(sv);
    std::fill_n(arr.data(), arr.num_elements(), 0);
}


std::vector<ClusterArrays::node_code_t> ClusterArrays::node_codes() const
{
    std::vector<ClusterArrays::node_code_t> ret;
    for (const auto& [code, arr] : m_na) {
        ret.push_back(code);
    }
    return ret;
}

std::vector<ClusterArrays::edge_code_t> ClusterArrays::edge_codes() const
{
    std::vector<ClusterArrays::edge_code_t> ret;
    for (const auto& [code, arr] : m_ea) {
        ret.push_back(code);
    }
    return ret;
}
const ClusterArrays::edge_array_t&
ClusterArrays::edge_array(ClusterArrays::edge_code_t ec) const
{
    static edge_array_t dummy;
    auto it = m_ea.find(ec);
    if (it == m_ea.end()) {
        THROW(KeyError() << errmsg{"no such edge code"});
    }
    return it->second;
}
ClusterArrays::edge_array_t&
ClusterArrays::edge_array(ClusterArrays::edge_code_t ec)
{
    return m_ea[ec];
}

const ClusterArrays::node_array_t&
ClusterArrays::node_array(ClusterArrays::node_code_t nc) const
{
    assert(nc != 'c');
    auto it = m_na.find(nc);
    if (it == m_na.end()) {
        THROW(KeyError() << errmsg{"no such node code: " + std::to_string(nc)});
    }
    return it->second;
}

ClusterArrays::node_array_t&
ClusterArrays::node_array(ClusterArrays::node_code_t nc)
{
    assert(nc != 'c');
    return m_na[nc];
}


ClusterArrays::store_address_t
ClusterArrays::vertex_address(cluster_vertex_t vtx)
{
    auto it = m_v2s.find(vtx);
    if (it == m_v2s.end()) {
        THROW(KeyError() << errmsg{"vertex has no address"});
    }
    return it->second;
}


ClusterArrays::node_row_t ClusterArrays::node_row(cluster_vertex_t vtx)
{
    const auto sa = vertex_address(vtx);
    // const auto& sa = m_v2s[vtx];
    typedef boost::multi_array_types::index_range range;
    return node_array(sa.code)[boost::indices[sa.index][range()]];
}

ClusterArrays::edge_code_t
ClusterArrays::edge_code(ClusterArrays::node_code_t nc1, ClusterArrays::node_code_t nc2)
{
    if (nc2 < nc1) {
        std::swap(nc1, nc2);
    }
    return ((nc1&0xff)<<8) | (nc2&0xff);
}
std::string ClusterArrays::edge_code_str(ClusterArrays::edge_code_t ec)
{
    std::string ret;
    ret.push_back((char)( (ec&0xff00) >> 8 ));
    ret.push_back((char)( (ec&0xff) ));
    return ret;
}


char ClusterArrays::node_code(const cluster_node_t& node)
{
    char nc = node.code();
    if (nc == 'c') return 'a';
    return nc;
}


// CAVEAT: BIG FAT BODGE HACK
//
// APPEND new c-nodes to represent activity nodes.  The old c-nodes
// will have their IChannel::pointer NULLED.  While the old c-nodes
// represent physical channels the new c-nodes represent activity in a
// channel AND in a slice.  These new c-nodes are used to fill the
// activity array and together hold the information in the "activity
// map" of the ISlice instances.  Old c-nodes with nullptr shoudl be
// ignored but are left in order not to invalidate vertex descriptors.
//
void ClusterArrays::bodge_channel_slice(cluster_graph_t& graph)
{
    std::vector<cluster_vertex_t> old_cvtx;
    for (const auto& vtx : vertex_range(graph)) {
        if ('c' == graph[vtx].code()) {
            old_cvtx.push_back(vtx);
        }
    }

    // Get known wire nodes
    std::unordered_map<IWire::pointer, cluster_vertex_t> wire_nodes;
    for (auto wvtx : vertex_range(graph)) {
        auto wnode = graph[wvtx];
        if (node_code(wnode) == 'w') {
            wire_nodes[std::get<wire_t>(wnode.ptr)] = wvtx;
        }
    }
    
    // Walk each slice.
    for (const auto& svtx : vertex_range(graph)) {
        const auto& snode = graph[svtx];
        if (node_code(snode) != 's') { continue; }

        // Add a new cnode for each activity.  Make c-w and c-s edges.
        // Remember new cnodes to later make c-m edges.
        std::unordered_map<IChannel::pointer, cluster_vertex_t> active_cnodes;
        const auto& islice = std::get<slice_t>(snode.ptr);
        auto activity = islice->activity(); // modifiable copy
        for (const auto& [ich, val] : activity) {
            auto cvtx = boost::add_vertex(cluster_node_t(ich), graph);
            active_cnodes[ich] = cvtx;
            boost::add_edge(cvtx, svtx, graph);

            for (const auto& iwire : ich->wires()) {
                auto wit = wire_nodes.find(iwire);
                cluster_vertex_t wvtx;
                if (wit == wire_nodes.end()) {
                    wvtx = boost::add_vertex(cluster_node_t(iwire), graph);
                    wire_nodes[iwire] = wvtx;
                }
                else {
                    wvtx = wit->second;
                }
                boost::add_edge(cvtx, wvtx, graph);
            }
        }

        // walk s-b-m to make new c-m edge
        for (auto bvtx : mir(boost::adjacent_vertices(svtx, graph))) {
            const auto& bnode = graph[bvtx];
            if (node_code(bnode) != 'b') { continue; }
            for (auto mvtx : mir(boost::adjacent_vertices(bvtx, graph))) {
                const auto& mnode = graph[mvtx];
                if (node_code(mnode) != 'm') { continue; }
                for (auto cvtx : mir(boost::adjacent_vertices(mvtx, graph))) {
                    const auto& cnode = graph[cvtx]; // old
                    if (node_code(cnode) != 'a') { continue; }
                    auto active_cvtx = active_cnodes[std::get<channel_t>(cnode.ptr)];
                    boost::add_edge(active_cvtx, mvtx, graph);
                }
            }
        }
    }

    // Invalidate old c-nodes.  Subsequent code much check for
    // non-null ptr value to avoid using these!
    for (auto cvtx : old_cvtx) {
        std::get<channel_t>(graph[cvtx].ptr) = nullptr;
    }
}

static bool is_old_chan(const cluster_node_t& node)
{
    if (node.code() == 'c') {
        if (std::get<channel_t>(node.ptr) == nullptr) {
            return true;
        }
    }
    return false;
}

static
void dump(const cluster_graph_t& graph)
{
    size_t nold_chan=0;
    std::unordered_map<char, std::unordered_set<int>> code2idents;
    for (const auto& vdesc : vertex_range(graph)) {
        const auto& node = graph[vdesc];
        if (is_old_chan(node)) {
            ++nold_chan;
            continue;
        }
        char code = node.code();
        int ident = node.ident();
        code2idents[code].insert(ident);
    }

    const std::map<std::string, size_t>& counts = count(graph);
    std::cerr << "nodes: ";
    for (const auto& [code,num] : counts) {
        if (code.size() != 1) { continue; }
        if (code[0] == 'c') {
            std::cerr << code << ":" << num-nold_chan << "(" << nold_chan << ") " ;
            continue;
        }
        std::cerr << code << ":" << num << " " ;
    }
    std::cerr << "edges: ";
    for (const auto& [code,num] : counts) {
        if (code.size() == 2) {
            std::cerr << code << ":" << num << " ";
        }
    }
    std::cerr << "\n";
}

void ClusterArrays::init(const cluster_graph_t& cin_graph)
{
    this->clear();

    // Remember original channel vertex descriptor
    for (const auto& vtx : vertex_range(cin_graph)) {
        const auto& node = cin_graph[vtx];
        auto nc = node.code();
        if (nc == 'c') {
            m_chid2desc[node.ident()] = vtx;
        }
    }

    cluster_graph_t mutable_graph = cin_graph;
    bodge_channel_slice(mutable_graph);
    const cluster_graph_t& graph = mutable_graph;

    {                           // debugging
        std::cerr << "pre-bodge:  ";
        dump(cin_graph);
        std::cerr << "post-bodge: ";
        dump(graph);
    }

    // Loop vertices to get per-type sizes.
    std::unordered_map<node_code_t, size_t> counts;
    for (const auto& vdesc : vertex_range(graph)) {
        const auto& node = graph[vdesc];
        if (is_old_chan(node)) {
            continue;
        }
        node_code_t code = node_code(node);
        ++counts[code];
    }

    // This hardwires the number of columns in the schema for each
    // node array type.  See ClusterArray.org for definitive
    // description and keep it in sync here.
    std::unordered_map<node_code_t, size_t> node_array_ncols = {
        // [desc, ident, value, uncertainty, index, wpid]
        {'a',1+3+2},            // 6
        // [desc, ident, wip, seg, ch, plane, [tail x,y,z], [head x,y,z]]
        {'w',1+11},             // 12
        // [desc,ident,sigv,sigu, faceid,sliceid,start,span, [3x2 wire bounds], nc,[12 corner y,z]]
        {'b',1+3+4+3*2+1+12*2}, // 39
        // [desc,ident,sigv,sigu, frameid, start, span]
        {'s',1+3+3},            // 7
        // [desc,ident,sigv,sigu, wpid]
        {'m',1+4},              // 5
    };

    // Allocate node arrays
    for (const auto& [code, count] : counts) {
        const std::vector<size_t> shape = {count, node_array_ncols[code]};
        rezero(node_array(code), shape);
    }

    // Loop vertices, build vtx<->row lookups and set ident column
    counts.clear();             // reuse to track nrows.
    for (const auto& vdesc : vertex_range(graph)) {
        const auto& node = graph[vdesc];
        if (is_old_chan(node)) {
            continue;
        }
        const node_code_t code = node_code(node);

        node_array_t::size_type row = counts[code];
        ++counts[code];

        m_v2s[vdesc] = store_address_t{code, row};

        node_array(code)[row][ident_col] = node.ident();
    }
    
    // One more time to process things that require graph traversal.
    for (const auto& vdesc : vertex_range(graph)) {
        const auto& node = graph[vdesc];
        if (is_old_chan(node)) {
            continue;
        }

        const node_code_t code = node_code(node);

        if (code == 's') { // slices and channels
            // s, part of b and c.
            // init_signals(graph, vdesc);
            init_slice(graph, vdesc);
            continue;
        }
        if (code == 'm') {
            init_measure(graph, vdesc);
            continue;
        }
        if (code == 'w') {
            init_wire(graph, vdesc);
            continue;
        }
        if (code == 'b') {
            init_blob(graph, vdesc);
            continue;
        }
    }

    // And the edges.  First to collect the data and learn the sizes.
    std::unordered_map<edge_code_t, size_t> ecounts;
    for (const auto& edge : mir(boost::edges(graph))) {
        auto tvtx = boost::source(edge, graph);
        auto hvtx = boost::target(edge, graph);

        const auto& tnode = graph[tvtx];
        if (is_old_chan(tnode)) continue;
        const auto& hnode = graph[hvtx];
        if (is_old_chan(hnode)) continue;

        auto tsa = vertex_address(tvtx);
        auto hsa = vertex_address(hvtx);

        // Does not need this as edge_code() does it internally.
        if (hsa.code < tsa.code) {
            std::swap(tsa, hsa);
            std::swap(tvtx, hvtx);
        }

        auto ecode = edge_code(tsa.code, hsa.code);
        ++ecounts[ecode];
    }
    for (const auto& [ecode, esize] : ecounts) {
        rezero(edge_array(ecode), {esize, 3});
    }

    // And again, this time with feeling!
    ecounts.clear();
    int edesc = 0;
    for (const auto& edge : mir(boost::edges(graph))) {
        auto tvtx = boost::source(edge, graph);
        auto hvtx = boost::target(edge, graph);

        const auto& tnode = graph[tvtx];
        if (is_old_chan(tnode)) continue;
        const auto& hnode = graph[hvtx];
        if (is_old_chan(hnode)) continue;

        auto tsa = vertex_address(tvtx);
        auto hsa = vertex_address(hvtx);

        // Do manual ordering to keep code/index correlated
        if (hsa.code < tsa.code) {
            std::swap(tsa, hsa);
            std::swap(tvtx, hvtx);
        }

        auto ecode = edge_code(tsa.code, hsa.code);
        edge_array_t::size_type erow = ecounts[ecode];
        ++ecounts[ecode];
        edge_array(ecode)[erow][desc_col] = edesc++;
        edge_array(ecode)[erow][tail_col] = tsa.index;
        edge_array(ecode)[erow][head_col] = hsa.index;
    }
}

// must process s+c together to spread activity onto channel
void ClusterArrays::init_slice(const cluster_graph_t& graph, cluster_vertex_t svtx)
{
    const auto& snode = graph[svtx];
    assert('s' == node_code(snode));

    const auto& islice = std::get<slice_t>(snode.ptr);
    const int sident = islice->ident();
    const auto& iframe = islice->frame();
    const int fident = iframe ? iframe->ident() : -1;
    const auto& activity = islice->activity();
        
    // The slices get a sum of all their activity.
    auto srow = node_row(svtx);

    srow[desc_col] = svtx;
    srow[ident_col] = sident;

    for (const auto& [_, sig] : activity) {
        srow[sigv_col] += sig.value();
        srow[sigu_col] += sig.uncertainty();
    }

    {   // slice start and span, just after sigv/sigu.
        node_array_t::size_type col = sigu_col;
        srow[++col] = fident;
        srow[++col] = islice->start();
        srow[++col] = islice->span();
    }
    
    // This is bodged activity
    for (auto cvtx : mir(boost::adjacent_vertices(svtx, graph))) {
        const auto& cnode = graph[cvtx];
        if (node_code(cnode) != 'a') { continue; }
        if (is_old_chan(cnode)) { continue; }

        auto ichan = std::get<channel_t>(cnode.ptr);
        const int chid = ichan->ident();
        auto sigit = activity.find(ichan);
        if (sigit == activity.end()) {
            continue;
        }
        auto cval = sigit->second;
        auto crow = node_row(cvtx);
        crow[desc_col] = m_chid2desc[chid];
        crow[ident_col] = chid;
        crow[sigv_col] = cval.value();
        crow[sigu_col] = cval.uncertainty();
        crow[sigu_col+1] = ichan->index();
        crow[sigu_col+2] = ichan->planeid().ident();
    }
}

void ClusterArrays::init_measure(const cluster_graph_t& graph, cluster_vertex_t mvtx)
{
    const auto& mnode = graph[mvtx];
    assert (node_code(mnode) == 'm');

    auto mrow = node_row(mvtx);
    auto imeas = std::get<meas_t>(mnode.ptr);
    auto mval = imeas->signal();
    int ident = imeas->ident();
    mrow[desc_col] = mvtx;
    mrow[ident_col] = ident;
    mrow[sigv_col] = mval.value();
    mrow[sigu_col] = mval.uncertainty();
    mrow[sigu_col+1] = imeas->planeid().ident();
}

void ClusterArrays::init_wire(const cluster_graph_t& graph, cluster_vertex_t wvtx)
{
    auto wrow = node_row(wvtx);

    const auto& wnode = graph[wvtx];
    assert('w' == node_code(wnode));

    const auto& iwire = get<wire_t>(wnode.ptr);
    const auto [p1,p2] = iwire->ray();

    int ident = iwire->ident();
    wrow[desc_col] = wvtx;
    wrow[ident_col] = ident;

    node_array_t::size_type col = ident_col;

    wrow[++col] = iwire->index();
    wrow[++col] = iwire->segment();
    wrow[++col] = iwire->channel();
    wrow[++col] = iwire->planeid().ident();

    wrow[++col] = p1.x();
    wrow[++col] = p1.y();
    wrow[++col] = p1.z();
    wrow[++col] = p2.x();
    wrow[++col] = p2.y();
    wrow[++col] = p2.z();
}

void ClusterArrays::init_blob(const cluster_graph_t& graph, cluster_vertex_t bvtx)
{
    const auto& bnode = graph[bvtx];
    assert('b' == node_code(bnode));
    auto brow = node_row(bvtx);
    const auto& iblob = get<blob_t>(bnode.ptr);
    const auto iface = iblob->face();
    const auto islice = iblob->slice();
    const auto& coords = iface->raygrid();

    const auto& shape = iblob->shape();
    const auto& strips = shape.strips();

    int ident = iblob->ident();
    brow[desc_col] = bvtx;
    brow[ident_col] = ident;
    brow[sigv_col] = iblob->value();
    brow[sigu_col] = iblob->uncertainty();

    node_array_t::size_type col = sigu_col + 1;

    // 3:faceid, 4:sident, 5:start, 6:span
    brow[col++] = iface->ident();
    brow[col++] = islice->ident();
    brow[col++] = islice->start();
    brow[col++] = islice->span();

    // 3x2 wire bounds: [7:13]
    for (size_t sind = 0; sind < 3; ++sind) {
        size_t strip_index = 2 + sind; // skip "virtual" layers
        const auto& bounds = strips[strip_index].bounds;

        brow[col++] = bounds.first;  // these are 
        brow[col++] = bounds.second; // WIP numbers
    }

    // 13, [14:38]
    //// corners is an awkward array.  Each of the 3 pairs of planes
    //// has 4 corners so a total of 12 but only those corners
    //// contained by each of the three "strips" that make up the blob
    //// are relevant and given.  Best we can do, I think is to use
    //// 1+2*12 = 25 columns.  First to given the number of corners,
    //// than that number of (y,z) pairs and zero pad any remainder.
    //// Alternatively we may define a second array type of shape 3 x
    //// N with columns (rowid, y, z).

    const auto& corners = shape.corners();
    brow[col++] = corners.size();
    for (const auto& c : corners) {
        auto pos = coords.ray_crossing(c.first, c.second);
        brow[col++] = pos[1];
        brow[col++] = pos[2];
    }
}


cluster_graph_t ClusterArrays::cluster_graph() const
{
    cluster_graph_t graph;



    return graph;
}
