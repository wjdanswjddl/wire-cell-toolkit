#include "WireCellAux/ClusterArrays.h"

#include "WireCellUtil/GraphTools.h"

#include <map>
#include <vector>
#include <unordered_map>

using namespace WireCell;
using namespace WireCell::Aux;
using namespace WireCell::GraphTools;


using channel_t = cluster_node_t::channel_t;
using wire_t = cluster_node_t::wire_t;
using blob_t = cluster_node_t::blob_t;
using slice_t = cluster_node_t::slice_t;
using meas_t = cluster_node_t::meas_t;

ClusterArrays::ClusterArrays(const cluster_graph_t& graph)
    : m_graph(graph)
{
    this->clear();              // not really needed on construction.
}


template<typename ArrayType>
void clear_array(ArrayType& arr)
{
    std::vector<size_t> extents(arr.num_dimensions(), 0);
    arr.resize(extents);
}
template<typename ArrayType>
size_t array_size(const ArrayType& arr)
{
    return arr.num_elements();
}
template<typename ArrayType>
bool is_empty(const ArrayType& arr)
{
    return 0 == array_size(arr);
}
template<typename ArrayType>
void rezero(ArrayType& arr, const std::vector<size_t>& sv)
{
    // resize and zero
    arr.resize(sv);
    std::fill_n(arr.data(), arr.num_elements(), 0);
}
template<typename ArrayType>
void reindex(ArrayType& arr, const std::vector<size_t>& sv)
{
    arr.reindex(sv);
}

void ClusterArrays::clear()
{
    m_node_codes.clear();

    clear_array(m_vdesc_indices);
    clear_array(m_vdescs);
    clear_array(m_edges);
    clear_array(m_node_ranges);
    clear_array(m_idents);
    clear_array(m_signals);
    clear_array(m_slice_durations);
    clear_array(m_wire_endpoints);
    clear_array(m_wire_addresses);
    clear_array(m_blob_shapes);
}


const std::string& ClusterArrays::node_codes() const
{
    if (! m_node_codes.empty()) {
        return m_node_codes;
    }
    const auto& vdescs = graph_vertices();
    const auto nnodes = array_size(vdescs);
    m_node_codes.resize(nnodes);
    for (size_t ind=0; ind<nnodes; ++ind) {
        m_node_codes[ind] = m_graph[vdescs[ind]].code();
    }
    return m_node_codes;
}


boost::iterator_range<boost::counting_iterator<size_t>>
ClusterArrays::ntype_range(char code) const
{
    size_t ind = cluster_node_t::code_index(code);
    if (ind >= cluster_node_t::known_codes.size()) {
        return boost::counting_range(std::size_t{0},std::size_t{0});
    }
    const auto& ranges = node_ranges();
    return index_range(ranges[ind]);
}

void ClusterArrays::core_nodes() const
{
    size_t nvtxs = array_size(m_vdescs);
    if (nvtxs) {
        return;
    }

    nvtxs = boost::num_vertices(m_graph);

    // Map ordering value to its vertex
    using ordering_t = int;
    using ordered_t = std::map<ordering_t, cluster_vertex_t>;
    // Map node type code to its set of ordered vertices
    using grouped_t = std::unordered_map<char, ordered_t>;

    grouped_t node_types;
    for (const auto& vdesc : vertex_range(m_graph)) {
        const auto& node = m_graph[vdesc];
        char code = node.code();
        ordering_t order = node.ident();
        node_types[code][order] = vdesc;
    }
    
    const size_t ntypes = cluster_node_t::known_codes.size();

    rezero(m_node_ranges, {ntypes, 2});
    rezero(m_vdescs, {nvtxs});
    rezero(m_vdesc_indices, {nvtxs});

    size_t full_index = 0;         // count up to Nnodes-1

    // Define the per-type partition and ordering
    for (size_t itype=0; itype < ntypes; ++itype) {
        const char code = cluster_node_t::known_codes[itype];
        const auto& of_type = node_types[code];
        const int noftype = of_type.size();

        m_node_ranges[itype][0] = full_index;
        m_node_ranges[itype][1] = noftype;

        for (const auto& [ord, vdesc] : of_type) {
            m_vdescs[full_index] = vdesc;
            m_vdesc_indices[vdesc] = full_index;
            ++full_index;
        }
    }
}

const ClusterArrays::vdesc_indices_t& ClusterArrays::vdesc_indices() const
{
    core_nodes();
    return m_vdesc_indices;
}

const ClusterArrays::vdescs_t& ClusterArrays::graph_vertices() const
{
    core_nodes();
    return m_vdescs;
}

const ClusterArrays::node_ranges_t& ClusterArrays::node_ranges() const
{
    core_nodes();
    return m_node_ranges;
}

const ClusterArrays::idents_t& ClusterArrays::idents() const
{
    if (! is_empty(m_idents)) {
        return m_idents;
    }
    const auto& vdescs = graph_vertices();
    const size_t nvtxs = array_size(vdescs);
    rezero(m_idents, {nvtxs});
    for (size_t ind=0; ind<nvtxs; ++ind) {
        const auto& vdesc = vdescs[ind];
        const auto& node = m_graph[vdesc];
        m_idents[ind] = node.ident();
    }
    return m_idents;
}


#include <iostream> // debug

const ClusterArrays::signals_t& ClusterArrays::signals() const
{
    if (! is_empty(m_signals)) {
        return m_signals;
    }

    const auto& vdescs = graph_vertices();
    const auto& vdinds = vdesc_indices();
    const size_t nvtxs = array_size(vdinds);
    rezero(m_signals, {nvtxs, 2});

    // std::cerr << "signals: codes:\n" <<  node_codes() << std::endl;

    // Slices we can traverse directly.  The rest are via graph
    // neighbors.
    for (auto sind : ntype_range('s')) {
        const auto svtx = vdescs[sind];
        const auto& snode = m_graph[svtx];
        assert('s' == snode.code());
        const auto& islice = std::get<slice_t>(snode.ptr);
        const auto& activity = islice->activity();
        
        for (const auto& [chid, sig] : activity) {
            m_signals[sind][0] += sig.value();
            m_signals[sind][1] += sig.uncertainty();
        }

        for (auto bvtx : mir(boost::adjacent_vertices(svtx, m_graph))) {
            const auto& bnode = m_graph[bvtx];
            if (bnode.code() != 'b') { continue; }

            auto bind = vdinds[bvtx];
            auto iblob = get<blob_t>(bnode.ptr);
            m_signals[bind][0] += iblob->value();
            m_signals[bind][1] += iblob->uncertainty();
            
            for (auto mvtx : mir(boost::adjacent_vertices(bvtx, m_graph))) {
                const auto& mnode = m_graph[mvtx];
                if (mnode.code() != 'm') { continue; }

                auto mind = vdinds[mvtx];

                for (auto cvtx : mir(boost::adjacent_vertices(mvtx, m_graph))) {
                    const auto& cnode = m_graph[cvtx];
                    if (mnode.code() != 'c') { continue; }

                    auto ichan = std::get<channel_t>(cnode.ptr);
                    auto sigit = activity.find(ichan);
                    if (sigit == activity.end()) {
                        continue;
                    }
                    auto cval = sigit->second;
                    auto cind = vdinds[cvtx];

                    m_signals[cind][0] += cval.value();
                    m_signals[cind][1] += cval.uncertainty();
                    m_signals[mind][0] += cval.value();
                    m_signals[mind][1] += cval.uncertainty();
                }
            }
        }
    }
    return m_signals;
}

const ClusterArrays::edges_t& ClusterArrays::edges() const
{
    if (! is_empty(m_edges)) {
        return m_edges;
    }

    // edges are given as pairs of node array indices and we want the
    // in this major-to-minor order:
    // (tail type, head type, tail instance, head instance)
    struct Edge {
        size_t tail_ind, head_ind;   // index in node array
        size_t tail_type, head_type; // type number in [1,6]
        int tail_ord, head_ord;      // ordering value
    };

    size_t nedges = boost::num_edges(m_graph);
    std::vector<Edge> edgedat(nedges);

    // pack edge data for sorting
    const auto& vdinds = vdesc_indices();
    size_t iedge=0;
    for (const auto& edge : mir(boost::edges(m_graph))) {
        auto tvtx = boost::source(edge, m_graph);
        auto hvtx = boost::target(edge, m_graph);

        const auto& tnode = m_graph[tvtx];
        const auto& hnode = m_graph[hvtx];

        edgedat[iedge++] = Edge{
            vdinds[tvtx], vdinds[hvtx],
            tnode.ptr.index(), hnode.ptr.index(),
            tnode.ident(), hnode.ident()};
    }

    // assure edge ordering
    std::sort(edgedat.begin(), edgedat.end(), [](const Edge& a, const Edge&b) {
        if (a.tail_type < b.tail_type) return true;
        if (a.tail_type > b.tail_type) return false;
        // tie
        if (a.head_type < b.head_type) return true;
        if (a.head_type > b.head_type) return false;
        // tie2
        if (a.tail_ord < b.tail_ord) return true;
        if (a.tail_ord > b.tail_ord) return false;
        return a.head_ord < b.head_ord;
    });
    
    // pack out
    rezero(m_edges, {nedges, 2});
    for (size_t ind=0; ind<nedges; ++ind) {
        m_edges[ind][0] = edgedat[ind].tail_ind;
        m_edges[ind][1] = edgedat[ind].head_ind;
    }
    return m_edges;
}

const ClusterArrays::slice_durations_t& ClusterArrays::slice_durations() const
{
    if (! is_empty(m_slice_durations)) {
        return m_slice_durations;
    }

    const size_t scode_index = cluster_node_t::code_index('s');
    const auto& slice_range = node_ranges()[scode_index];
    rezero(m_slice_durations, {slice_range[1], 2});
    reindex(m_slice_durations, {slice_range[0], 0});
    // it's now indexed with absolute indices

    const auto& vdescs = graph_vertices();
    for (auto sind : index_range(slice_range)) {
        const auto svtx = vdescs[sind];
        const auto& snode = m_graph[svtx];

        const auto& islice = get<slice_t>(snode.ptr);
        m_slice_durations[sind][0] = islice->start();
        m_slice_durations[sind][1] = islice->span();
    }
    reindex(m_slice_durations, {0,0});
    // it's now indexed with relative indices starting at 0
    return m_slice_durations;
}

const ClusterArrays::wire_endpoints_t& ClusterArrays::wire_endpoints() const
{
    if (is_empty(m_wire_endpoints)) {
        core_wires();
    }
    return m_wire_endpoints;
}

const ClusterArrays::wire_addresses_t& ClusterArrays::wire_addresses() const
{
    if (is_empty(m_wire_addresses)) {
        core_wires();
    }
    return m_wire_addresses;
}

void ClusterArrays::core_wires() const
{
    const size_t wcode_index = cluster_node_t::code_index('w');
    const auto& wire_range = node_ranges()[wcode_index];
    const size_t wire0 = wire_range[0];
    const size_t nwires = wire_range[1];
    rezero(m_wire_endpoints, {nwires, 2, 3});
    rezero(m_wire_addresses, {nwires, 4});

    const auto& vdescs = graph_vertices();
    for (size_t ind=0; ind< nwires; ++ind) {
        size_t wind = ind + wire0; // relative index

        const auto wvtx = vdescs[wind];
        const auto& wnode = m_graph[wvtx];

        const auto& iwire = get<wire_t>(wnode.ptr);
        const auto [p1,p2] = iwire->ray();
        m_wire_endpoints[ind][0][0] = p1.x();
        m_wire_endpoints[ind][0][1] = p1.y();
        m_wire_endpoints[ind][0][2] = p1.z();
        m_wire_endpoints[ind][1][0] = p2.x();
        m_wire_endpoints[ind][1][1] = p2.y();
        m_wire_endpoints[ind][1][2] = p2.z();

        m_wire_addresses[ind][0] = iwire->index();
        m_wire_addresses[ind][1] = iwire->segment();
        m_wire_addresses[ind][2] = iwire->channel();
        m_wire_addresses[ind][3] = iwire->planeid().ident();
    }
}


const ClusterArrays::blob_shapes_t& ClusterArrays::blob_shapes() const
{
    if (! is_empty(m_blob_shapes)) {
        return m_blob_shapes;
    }

    const size_t bcode_index = cluster_node_t::code_index('b');
    const auto& blob_range = node_ranges()[bcode_index];
    const size_t blob0 = blob_range[0];
    const size_t nblobs = blob_range[1];
    rezero(m_blob_shapes, {nblobs, 3, 2});

    const auto& vdescs = graph_vertices();
    for (size_t ind=0; ind< nblobs; ++ind) {
        size_t bind = ind + blob0;

        const auto bvtx = vdescs[bind];
        const auto& bnode = m_graph[bvtx];

        const auto& iblob = get<blob_t>(bnode.ptr);
        const auto& strips = iblob->shape().strips();

        for (size_t sind = 0; sind < 3; ++sind) {
            size_t strip_index = 2 + sind; // skip "virtual" layers
            const auto& bounds = strips[strip_index].bounds;
            m_blob_shapes[ind][sind][0] = bounds.first;  // these are 
            m_blob_shapes[ind][sind][1] = bounds.second; // WIP indices
        }
    }
    return m_blob_shapes;
}
