#include "WireCellAux/ClusterArrays.h"

#include "WireCellUtil/GraphTools.h"

#include <map>

using namespace WireCell;
using namespace WireCell::Aux;
using namespace WireCell::GraphTools;


using channel_t = cluster_node_t::channel_t;
using wire_t = cluster_node_t::wire_t;
using blob_t = cluster_node_t::blob_t;
using slice_t = cluster_node_t::slice_t;
using meas_t = cluster_node_t::meas_t;

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
        return dummy;
    }
    return it->second;
}
const ClusterArrays::node_array_t&
ClusterArrays::node_array(ClusterArrays::node_code_t nc) const
{
    static node_array_t dummy;
    auto it = m_na.find(nc);
    if (it == m_na.end()) {
        return dummy;
    }
    return it->second;
}



// Some hard-wired schema values
// See ClusterArray.org docs and keep in sync.
// For the common node array columns
static const size_t ident_col=0; // all node types
static const size_t sigv_col=1;  // all but wires
static const size_t sigu_col=2;  // all but wires


ClusterArrays::node_row_t ClusterArrays::node_row(cluster_vertex_t vtx)
{
    const auto& sa = m_v2s[vtx];
    typedef boost::multi_array_types::index_range range;
    return m_na[sa.code][boost::indices[sa.index][range()]];
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
void ClusterArrays::init(const cluster_graph_t& graph)
{
    this->clear();

    // Loop vertices to get per-type sizes.
    std::unordered_map<node_code_t, size_t> counts;
    for (const auto& vdesc : vertex_range(graph)) {
        const auto& node = graph[vdesc];
        node_code_t code = node.code();
        ++counts[code];
    }

    // This hardwires the number of columns in the schema for each
    // node array type.  See ClusterArray.org for definitive
    // description and keep it in sync here.
    std::unordered_map<node_code_t, size_t> node_array_ncols = {
        {'c',3}, {'w',11}, {'b',9}, {'s',5}, {'m', 3},
    };

    // Allocate node arrays
    for (const auto& [code, count] : counts) {
        const std::vector<size_t> shape = {count, node_array_ncols[code]};
        rezero(m_na[code], shape);
    }

    // Loop vertices, build vtx<->row lookups and set ident column
    counts.clear();             // reuse to track nrows.
    for (const auto& vdesc : vertex_range(graph)) {
        const auto& node = graph[vdesc];
        const node_code_t code = node.code();

        node_array_t::size_type row = counts[code];
        ++counts[code];

        m_v2s[vdesc] = store_address_t{code, row};

        m_na[code][row][ident_col] = node.ident();
    }
    
    // One more time to process things that require graph traversal.
    for (const auto& vdesc : vertex_range(graph)) {
        const auto& node = graph[vdesc];
        const node_code_t code = node.code();

        if (code == 's') {      // also does 'm' and 'c' 
            init_signals(graph, vdesc);
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

        auto tsa = m_v2s[tvtx];
        auto hsa = m_v2s[hvtx];

        auto ecode = edge_code(tsa.code, hsa.code);
        ++ecounts[ecode];
    }
    for (const auto& [ecode, esize] : ecounts) {
        rezero(m_ea[ecode], {esize, 2});
    }

    // And again, this time with feeling!
    ecounts.clear();
    for (const auto& edge : mir(boost::edges(graph))) {
        auto tvtx = boost::source(edge, graph);
        auto hvtx = boost::target(edge, graph);

        auto tsa = m_v2s[tvtx];
        auto hsa = m_v2s[hvtx];

        // Do manual ordering to keep code/index correlated
        if (hsa.code > tsa.code) {
            std::swap(tsa, hsa);
        }
        auto ecode = edge_code(tsa.code, hsa.code);
        edge_array_t::size_type erow = ecounts[ecode];
        ++ecounts[ecode];
        m_ea[ecode][erow][0] = tsa.index;
        m_ea[ecode][erow][1] = hsa.index;
    }
}


void ClusterArrays::init_signals(const cluster_graph_t& graph, cluster_vertex_t svtx)
{
    const auto& snode = graph[svtx];
    assert('s' == snode.code());
    const auto& islice = std::get<slice_t>(snode.ptr);
    const auto& activity = islice->activity();
        
    // The slices get a sum of all their activity.
    auto srow = node_row(svtx);
    for (const auto& [chid, sig] : activity) {
        srow[sigv_col] += sig.value();
        srow[sigu_col] += sig.uncertainty();
    }
    {   // slice start and span, just after sigv/sigu.
        node_array_t::size_type col = sigu_col;
        srow[++col] = islice->start();
        srow[++col] = islice->span();
    }
    for (auto bvtx : mir(boost::adjacent_vertices(svtx, graph))) {
        const auto& bnode = graph[bvtx];
        if (bnode.code() != 'b') { continue; }

        auto brow = node_row(bvtx);
        auto iblob = get<blob_t>(bnode.ptr);
        brow[sigv_col] += iblob->value();
        brow[sigu_col] += iblob->uncertainty();
            
        for (auto mvtx : mir(boost::adjacent_vertices(bvtx, graph))) {
            const auto& mnode = graph[mvtx];
            if (mnode.code() != 'm') { continue; }

            auto mrow = node_row(mvtx);

            for (auto cvtx : mir(boost::adjacent_vertices(mvtx, graph))) {
                const auto& cnode = graph[cvtx];
                if (cnode.code() != 'c') { continue; }
                
                auto ichan = std::get<channel_t>(cnode.ptr);
                auto sigit = activity.find(ichan);
                if (sigit == activity.end()) {
                    continue;
                }
                auto cval = sigit->second;

                auto crow = node_row(cvtx);
                crow[sigv_col] += cval.value();
                crow[sigu_col] += cval.uncertainty();
                mrow[sigv_col] += cval.value();
                mrow[sigu_col] += cval.uncertainty();
            }
        }
    }

}

void ClusterArrays::init_wire(const cluster_graph_t& graph, cluster_vertex_t wvtx)
{
    auto wrow = node_row(wvtx);

    const auto& wnode = graph[wvtx];

    const auto& iwire = get<wire_t>(wnode.ptr);
    const auto [p1,p2] = iwire->ray();

    // Wires have ident col=0 set above, see ClusterArrays.org.
    node_array_t::size_type col = 1;
    wrow[col++] = iwire->index();
    wrow[col++] = iwire->segment();
    wrow[col++] = iwire->channel();
    wrow[col++] = iwire->planeid().ident();

    wrow[col++] = p1.x();
    wrow[col++] = p1.y();
    wrow[col++] = p1.z();
    wrow[col++] = p2.x();
    wrow[col++] = p2.y();
    wrow[col++] = p2.z();
}

void ClusterArrays::init_blob(const cluster_graph_t& graph, cluster_vertex_t bvtx)
{
    auto brow = node_row(bvtx);
    const auto& bnode = graph[bvtx];
    const auto& iblob = get<blob_t>(bnode.ptr);
    const auto& strips = iblob->shape().strips();

    for (size_t sind = 0; sind < 3; ++sind) {
        size_t strip_index = 2 + sind; // skip "virtual" layers
        const auto& bounds = strips[strip_index].bounds;

        // [ident,sigv,sigu] + pairs
        node_array_t::size_type col = 3 + sind*2;
        brow[col++] = bounds.first;  // these are 
        brow[col++] = bounds.second; // WIP numbers
    }

}

