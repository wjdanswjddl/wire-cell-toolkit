#include "WireCellImg/CSGraph.h"

#include "WireCellUtil/Exceptions.h"
#include "WireCellIface/SimpleBlob.h"

using namespace WireCell;
using namespace WireCell::Img;
using namespace WireCell::Img::CS;


int CS::ordering(const meas_node_t& m)
{
    int order = 0x7fffffff;
    for (const auto& one : *m) {
        int ident = one->ident();
        if (ident < order) {
            order = ident;
        }
    }
    return order;
}

int CS::ordering(const blob_node_t& blob)
{
    // This assumes that a context sets ident uniquely.
    return blob->ident();
}

value_t CS::measure_sum(const meas_node_t& imeas,
                        const slice_node_t& islice)
{
    // Use double for the sum for a little extra precision.
    WireCell::Measurement::float64 value = 0;

    const auto& activity = islice->activity();
    for (const auto& ich : *(imeas.get())) {
        const auto& it = activity.find(ich);
        if (it == activity.end()) {
            continue;
        }
        value += it->second;
    }
    return value_t(value);
}

indexed_vdescs_t CS::select_ordered(const graph_t& csg,
                                    node_t::Kind kind)
{
    std::vector<vdesc_t> ret;
    for (const auto& v : vertex_range(csg)) {
        const auto& vp = csg[v];
        if (vp.kind == kind) {
            ret.push_back(v);
        }
    }
    std::sort(ret.begin(), ret.end(), [&](const vdesc_t& a, const vdesc_t& b) {
        return csg[a].ordering < csg[b].ordering;
    });
    return indexed_vdescs_t(ret);
}

graph_t CS::solve(const graph_t& csg, const SolveParams& params)
{
    graph_t csg_out;

    // Copy graph level properties
    csg_out[boost::graph_bundle] = csg[boost::graph_bundle];

    IndexedSet<vdesc_t> blob_desc_out;
    auto blob_desc = select_ordered(csg, node_t::blob);

    const size_t nblob = blob_desc.size();
    double_vector_t source = double_vector_t::Zero(nblob);
    double_vector_t weight = double_vector_t::Zero(nblob);
    for (size_t bind=0; bind<nblob; ++bind) {
        const auto& blob_in = csg[blob_desc.collection[bind]];

        const auto valerr = blob_in.value;
        source(bind) = valerr.value();
        weight(bind) = valerr.uncertainty();

        auto desc_out = boost::add_vertex(blob_in, csg_out);
        blob_desc_out(desc_out);        
    }

    IndexedSet<vdesc_t> meas_desc_out;
    auto meas_desc = select_ordered(csg, node_t::meas);

    const size_t nmeas = meas_desc.size();
    double_vector_t measure = double_vector_t::Zero(nmeas);
    double_matrix_t mcov = double_matrix_t::Zero(nmeas, nmeas);
    for (size_t mind=0; mind<nmeas; ++mind) {
        const auto& meas_in = csg[meas_desc.collection[mind]];
        const auto valerr = meas_in.value;
        measure(mind) = valerr.value();
        mcov(mind, mind) = valerr.uncertainty();

        auto desc_out = boost::add_vertex(meas_in, csg_out);
        meas_desc_out(desc_out);        
    }

    double_matrix_t A = double_matrix_t::Zero(meas_desc.size(), blob_desc.size());

    for (auto [ei, ei_end] = boost::edges(csg); ei != ei_end; ++ei) {
        const vdesc_t tail = boost::source(*ei, csg);
        const vdesc_t head = boost::target(*ei, csg);

        const auto kind1 = csg[tail].kind;
        const auto kind2 = csg[head].kind;
        int bind{0}, mind{0};
        if (kind1 == node_t::blob and kind2 == node_t::meas) {
            bind = blob_desc.get(tail);
            mind = meas_desc.get(head);
        }        
        else if (kind2 == node_t::blob and kind1 == node_t::meas) {
            bind = blob_desc.get(head);
            mind = meas_desc.get(tail);
        }
        else {
            // someone has violated my requirements with this edge!
            continue;
        }
        A(mind, bind) = 1;

        boost::add_edge(blob_desc_out.collection[bind],
                        meas_desc_out.collection[mind],
                        csg_out);
    }

    Eigen::LLT<double_matrix_t> llt(mcov.inverse());
    double_matrix_t U = llt.matrixL().transpose();

    // The measure vector in a "whitened" basis
    double_vector_t m_white = params.scale*U*measure;

    // The blob-measure association in "whitened" basis (becomes
    // the "reasponse" matrix in ress solving).
    double_matrix_t R_white = U * A;

    auto solution = Ress::solve(R_white, m_white, params.ress,
                                source, weight);
    auto predicted = Ress::predict(R_white, solution);

    auto& gp_out = csg_out[boost::graph_bundle];
    gp_out.chi2_base = Ress::chi2_base(m_white, predicted);
    gp_out.chi2_l1 = Ress::chi2_l1(m_white, solution, params.ress.lambda);

    // Update outgoing blob nodes with their solution
    for (size_t ind=0; ind<nblob; ++ind) {
        auto& bvalue = csg_out[blob_desc_out.collection[ind]];
        bvalue.value.value(solution[ind]);
    }
    return csg_out;
}


graph_t CS::unpack_slice(const cluster_graph_t& cgraph,
                         cluster_vertex_t sd,
                         const value_t& meas_thresh)
{
    graph_t slice_graph;

    const auto svtx = cgraph[sd];
    if (svtx.code() != 's') {
        return slice_graph;
    }
    slice_node_t islice = std::get<slice_node_t>(svtx.ptr);

    slice_graph[boost::graph_bundle].islice = islice;
    slice_graph[boost::graph_bundle].index = islice->ident();

    // get the blobs's
    for (auto sb_edge : mir(boost::out_edges(sd, cgraph))) {
        cluster_vertex_t bd = boost::target(sb_edge, cgraph);
        const auto& bvtx = cgraph[bd];
        if (bvtx.code() != 'b') { continue; }
                
        auto bptr = std::get<blob_node_t>(bvtx.ptr);
        node_t blob{bd, node_t::blob, ordering(bptr), value_t(0,1)};
        auto blob_desc = boost::add_vertex(blob, slice_graph);

        // collect the blobs's and the measures's
        int nsaved = 0;
        for (auto bx_edge : mir(boost::out_edges(bd, cgraph))) {
            cluster_vertex_t xd = boost::target(bx_edge, cgraph);
            const auto& xvtx = cgraph[xd];
            char code = xvtx.code();
            if (code != 'm') {
                continue;
            }
            auto mptr = std::get<meas_node_t>(xvtx.ptr);
            const auto msum = measure_sum(mptr, islice);
            if (msum.value() < meas_thresh.value() or
                msum.uncertainty() > meas_thresh.uncertainty()) {
                continue;
            }
            node_t meas{xd, node_t::meas, ordering(mptr), msum};
                    
            auto meas_desc = boost::add_vertex(meas, slice_graph);
            boost::add_edge(blob_desc, meas_desc, slice_graph);
            ++nsaved;
        }

        // In principle, all measures supporting a blob can fail the
        // threshold.
        if (!nsaved) {
            boost::remove_vertex(blob_desc, slice_graph);
        }
    }
    return slice_graph;
}


void CS::connected_subgraphs(const graph_t& slice_graph,
                             std::back_insert_iterator<graph_vector_t> subgraphs_out)
{
    auto islice = slice_graph[boost::graph_bundle].islice;
    if (!islice) {
        THROW(ValueError() << errmsg{"Slice graph has no slice"});
    }

    std::unordered_map<int, std::vector<vdesc_t> > groups;
    std::unordered_map<vdesc_t, int> desc2id;
    boost::connected_components(slice_graph, boost::make_assoc_property_map(desc2id));
    for (auto& [desc,id] : desc2id) {  // invert
        groups[id].push_back(desc);
    }
    for (const auto& [index, slice_vdescs] : groups) {
        graph_t sub_graph;
        sub_graph[boost::graph_bundle].islice = islice;
        sub_graph[boost::graph_bundle].index = index;
        std::unordered_map<vdesc_t, vdesc_t> slice2sub;
        // Add map from slice to sub graph vdesc's
        for (const auto& slice_vdesc : slice_vdescs) {
            vdesc_t sub_vdesc = boost::add_vertex(slice_graph[slice_vdesc], sub_graph);
            slice2sub[slice_vdesc] = sub_vdesc;
        }
        // Add edges
        for (const auto& slice_vdesc : slice_vdescs) {
            for (auto edge : mir(boost::out_edges(slice_vdesc, slice_graph))) {
                auto tail = slice2sub[boost::source(edge, slice_graph)];
                auto head = slice2sub[boost::target(edge, slice_graph)];
                boost::add_edge(tail, head, sub_graph);
            }
        }
        subgraphs_out = sub_graph;
    }
}

void CS::unpack(const cluster_graph_t& cgraph,
                std::back_insert_iterator<graph_vector_t> subgraphs,
                const value_t& meas_thresh)
{
    // get the slices's
    for (const auto& sd : vertex_range(cgraph)) {
        const auto svtx = cgraph[sd];
        if (svtx.code() != 's') {
            continue;
        }
        auto slice_graph = unpack_slice(cgraph, sd, meas_thresh);
        if (! boost::num_vertices(slice_graph)) {
            continue;
        }
        // log->debug("slice_graph {}: nvertices={} nedges={}",
        //            slice_graph[boost::graph_bundle].index,
        //            boost::num_vertices(slice_graph),
        //            boost::num_edges(slice_graph));
        connected_subgraphs(slice_graph, subgraphs);
    }
}

// Replace the IBlob pointer with one differing only in its value...
void replace_blob_value(value_t nv, cluster_vertex_t v, cluster_graph_t& g)
{
    IBlob::pointer ob = get<IBlob::pointer>(g[v].ptr);
    auto nb = std::make_shared<SimpleBlob>(
        ob->ident(), nv.value(), nv.uncertainty(), 
        ob->shape(), ob->slice(), ob->face());
    g[v].ptr = nb;    
}

// The csgs provide b-m graph with vdesc into cgraph.  These {b} and
// {m} sets are subsets of those from the cgraph.  We must replace
// IBlobs on b nodes, remove b and m but leave the rest of the cgraph
// as-is.
cluster_graph_t CS::repack(const cluster_graph_t& cgin,
                           const std::vector<graph_t>& csgs)
{
    cluster_graph_t cgout;
    boost::copy_graph(cgin, cgout);

    // holds original b or m from the csgs.
    std::unordered_set<cluster_vertex_t> live;

    for (const graph_t& csg : csgs) {
        for (auto v : vertex_range(csg)) {
            const auto& node = csg[v];
            live.insert(node.orig_desc);
            if (node.kind == node_t::blob) {
                replace_blob_value(node.value, node.orig_desc, cgout);
            }
        }
    }

    // Remove any b's or m's not in live
    for (auto v : vertex_range(cgout)) {
        auto& node = cgout[v];
        char code = node.code();
        if (code == 'b' or code == 'm') {
            if (live.count(v)) {
                continue;
            }
            boost::clear_vertex(v, cgout);
            boost::remove_vertex(v, cgout);
        }
    }

    // And one more time to remove any neighborless b's or m's
    for (auto v : vertex_range(cgout)) {
        auto& node = cgout[v];
        char code = node.code();
        if (code == 'b' or code == 'm') {
            const auto& [eit, end] = boost::out_edges(v, cgout);
            if (eit == end) {
                boost::remove_vertex(v, cgout);
            }
        }
    }

    return cgout;
}

