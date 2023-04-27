#include "WireCellImg/Projection2D.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/GraphTools.h"
#include "WireCellUtil/Array.h"
#include "WireCellUtil/Stream.h"
#include "WireCellUtil/String.h"

#include <fstream>
#include <unordered_set>
#include <utility>

using namespace WireCell;
using namespace WireCell::Img::Projection2D;

// Here, "node" implies a cluster graph vertex payload object.
using channel_t = cluster_node_t::channel_t;
using wire_t = cluster_node_t::wire_t;
using blob_t = cluster_node_t::blob_t;
using slice_t = cluster_node_t::slice_t;
using meas_t = cluster_node_t::meas_t;


// template <typename It> boost::iterator_range<It> mir(std::pair<It, It> const& p) {
//     return boost::make_iterator_range(p.first, p.second);
// }
// template <typename It> boost::iterator_range<It> mir(It b, It e) {
//     return boost::make_iterator_range(b, e);
// }

// std::vector<cluster_vertex_t> WireCell::Img::Projection2D::neighbors(const WireCell::cluster_graph_t& cg, const cluster_vertex_t& vd)
// {
//     std::vector<cluster_vertex_t> ret;
//     for (auto edge : boost::make_iterator_range(boost::out_edges(vd, cg))) {
//         cluster_vertex_t neigh = boost::target(edge, cg);
//         ret.push_back(neigh);
//     }
//     return ret;
// }

// template <typename Type>
// std::vector<cluster_vertex_t> WireCell::Img::Projection2D::neighbors_oftype(const WireCell::cluster_graph_t& cg, const cluster_vertex_t& vd)
// {
//     std::vector<cluster_vertex_t> ret;
//     for (const auto& vp : neighbors(cg, vd)) {
//         if (std::holds_alternative<Type>(cg[vp].ptr)) {
//             ret.push_back(vp);
//         }
//     }
//     return ret;
// }

std::unordered_map<int, std::set<cluster_vertex_t> > WireCell::Img::Projection2D::get_geom_clusters(const WireCell::cluster_graph_t& cg)
{
    std::unordered_map<int, std::set<cluster_vertex_t> > groups;
    cluster_graph_t cg_blob;
    
    size_t nblobs = 0;
    std::unordered_map<cluster_vertex_t, cluster_vertex_t> old2new;
    std::unordered_map<cluster_vertex_t, cluster_vertex_t> new2old;
    for (const auto& vtx : GraphTools::mir(boost::vertices(cg))) {
        const auto& node = cg[vtx];
        if (node.code() == 'b') {
            ++nblobs;
            auto newvtx = boost::add_vertex(node, cg_blob);
            old2new[vtx] = newvtx;
            new2old[newvtx] = vtx;
        }
    }
    // debug
    std::cout << "nblobs: " << nblobs << std::endl;
    
    if (!nblobs) {
        return groups;
    }

    for (auto edge : GraphTools::mir(boost::edges(cg))) {
        auto old_tail = boost::source(edge, cg);
        auto old_head = boost::target(edge, cg);

        auto old_tit = old2new.find(old_tail);
        if (old_tit == old2new.end()) {
            continue;
        }
        auto old_hit = old2new.find(old_head);
        if (old_hit == old2new.end()) {
            continue;
        }
        const auto& hnode = cg_blob[old_hit->second];
        const auto& tnode = cg_blob[old_tit->second];
        if (hnode.code() == 'b' && tnode.code() == 'b') {
            boost::add_edge(old_tit->second, old_hit->second, cg_blob);
        }
    }

    // for debugging, return as one group
    // for (const auto& desc : GraphTools::mir(boost::vertices(cg_blob))) {
    //     groups[0].insert(new2old[desc]);
    // }
    // return groups;

    std::unordered_map<cluster_vertex_t, int> desc2id;
    boost::connected_components(cg_blob, boost::make_assoc_property_map(desc2id));
    for (auto& [desc,id] : desc2id) {  // invert
        groups[id].insert(new2old[desc]);
    }

    // debug
    // for (auto id2desc : groups) {
    //     auto & descvec = id2desc.second;
    //     if (descvec.size()>10) {
    //         std::cout << id2desc.first << ": ";
    //         for (auto &desc : descvec) {
    //             std::cout << desc << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    // }

    return groups;
}

struct pair_hash
{
    template <class T1, class T2>
    std::size_t operator () (std::pair<T1, T2> const &pair) const
    {
        std::size_t h1 = std::hash<T1>()(pair.first);
        std::size_t h2 = std::hash<T2>()(pair.second);
        return h1 ^ h2;
    }
};

LayerProjection2DMap WireCell::Img::Projection2D::get_projection(
    const WireCell::cluster_graph_t& cg, const std::set<cluster_vertex_t>& group, const size_t nchan, const size_t nslice)
{
    using triplet_t = Eigen::Triplet<scaler_t>;
    using triplet_vec_t = std::vector<triplet_t>;
    std::unordered_map<WirePlaneLayer_t, triplet_vec_t> lcoeff;
    std::unordered_set<std::pair<int, int>, pair_hash> filled;
    // layer_projection_map_t ret;
    LayerProjection2DMap ret;

    // assumes one blob linked to one slice
    // use b-w-c to find all channels linked to the blob
    for (const auto& blob_desc : group) {
        const auto& node = cg[blob_desc];
        if (node.code() == 'b') {
            const auto slice_descs = neighbors_oftype<slice_t>(cg, blob_desc);
            if (slice_descs.size() != 1) {
                THROW(ValueError() << errmsg{"slice_descs.size()!=1"});
            }
            auto slice = std::get<slice_t>(cg[slice_descs.front()].ptr);
            // TODO: make this tick duration configurable
            int start = int(slice->start() / (500 * units::nanosecond));
            int span = int(slice->span() / (500 * units::nanosecond));
            auto activity = slice->activity();
            const auto wire_descs = neighbors_oftype<wire_t>(cg, blob_desc);
            // std::cout << " #wire " << wire_descs.size();
            for (const auto wire_desc : wire_descs) {
                const auto chan_descs = neighbors_oftype<channel_t>(cg, wire_desc);
                for (const auto chan_desc : chan_descs) {
                    auto chan = std::get<channel_t>(cg[chan_desc].ptr);
                    WirePlaneLayer_t layer = chan->planeid().layer();
                    int index = chan->ident();
                    auto val = activity[chan].value();
                    auto unc = activity[chan].uncertainty();
                    auto charge = val;
                    if (unc > 1e10) { // TODO: make this configurable
                        charge = 1e-12;
                    }
                    // if filled, skip
                    if (filled.find({index,start})!=filled.end()) {
                        continue;
                    }
                    // TODO: validate this
                    lcoeff[layer].push_back({index, start, charge});
                    filled.insert({index,start});
                    // DEBUGONLY
                    // if(index == 6486 && start == 8066) {
                    //     IBlob::pointer blob = std::get<blob_t>(node.ptr);
                    //     std::cout
                    //     << " blob_desc: " << blob_desc
                    //     << " blob->ident(): " << blob->ident()
                    //     << " blob->shape(): " << blob->shape().as_string()
                    //     << " slice->ident(): " << slice->ident()
                    //     << " index: " << index
                    //     << " start: " << start
                    //     << " charge: " << charge
                    //     << std::endl;
                    // }
                    // auto& bound = ret.m_layer_proj[layer].m_bound;
                    // if (index < std::get<0>(bound)) std::get<0>(bound) = index;
                    // if (index > std::get<1>(bound)) std::get<1>(bound) = index;
                    // if (start < std::get<2>(bound)) std::get<2>(bound) = start;
                    // if (start > std::get<3>(bound)) std::get<3>(bound) = start;
                }
            }
            // std::cout << std::endl;
        }
    }

    for (auto lc : lcoeff) {
        auto l = lc.first;
        auto c = lc.second;
        if(ret.m_layer_proj.find(l)==ret.m_layer_proj.end()) {
            ret.m_layer_proj.insert({l, Projection2D(nchan, nslice)});
        }
        ret.m_layer_proj[l].m_proj.setFromTriplets(c.begin(), c.end());
    }
    return ret;
}

std::string WireCell::Img::Projection2D::dump(const Projection2D& proj2d, bool verbose) {
    std::stringstream ss;
    ss << "Projection2D:";
    // << " {" << std::get<0>(proj2d.m_bound)
    // << "," << std::get<1>(proj2d.m_bound)
    // << "," << std::get<2>(proj2d.m_bound)
    // << "," << std::get<3>(proj2d.m_bound)
    // << "}";

    auto ctq = proj2d.m_proj;
    size_t counter = 0;
    for (int k=0; k<ctq.outerSize(); ++k) {
        for (sparse_mat_t::InnerIterator it(ctq,k); it; ++it)
        {
            if (verbose) {
                ss << " {" << it.row() << ", " << it.col() << "}->" << it.value();
            }
            ++counter;
        }
    }
    ss << " #: " << counter;
    
    return ss.str();
}

bool WireCell::Img::Projection2D::write(const Projection2D& proj2d, const std::string& fname)
{
    using namespace WireCell::Stream;
    boost::iostreams::filtering_ostream fout;
    fout.clear();
    output_filters(fout, fname);
    Eigen::MatrixXf dense_m(proj2d.m_proj);
    Array::array_xxf dense_a = dense_m.array();
    const std::string aname = String::format("proj2d_%d.npy", 0);
    WireCell::Stream::write(fout, aname, dense_a);
    fout.flush();
    fout.pop();
    return 0;
}

// return a sparse matrix mask for elements that are larger than th(reshold)
sparse_mat_t mask (const sparse_mat_t& sm, const float th = 0) {
    sparse_mat_t smm = sm;
    for (int k=0; k<smm.outerSize(); ++k) {
        for (sparse_mat_t::InnerIterator it(smm,k); it; ++it)
        {
            if (it.value() > th) {
                it.valueRef() = 1;
            }
        }
    }
    return smm;
}

scaler_t loop_sum (const sparse_mat_t& sm, std::function<scaler_t(scaler_t)> f) {
    scaler_t ret = 0;

    for (int k=0; k<sm.outerSize(); ++k) {
        for (sparse_mat_t::InnerIterator it(sm,k); it; ++it)
        {
            ret += f(it.value());
        }
    }
    return ret;
}

bool loop_exist (const sparse_mat_t& sm, std::function<bool(scaler_t)> f) {
    for (int k=0; k<sm.outerSize(); ++k) {
        for (sparse_mat_t::InnerIterator it(sm,k); it; ++it)
        {
            if (f(it.value())) {
                return true;
            }
        }
    }
    return false;
}

// 1: tar is part of ref
// -1: ref is part of tar
// 2: tar is equal to ref
// -2: tar is equal to ref and both are empty
// 0 ref and tar do not belong to each other
int WireCell::Img::Projection2D::judge_coverage(const Projection2D& ref, const Projection2D& tar) {
    // ref * tar
    sparse_mat_t ref_t_tar = ref.m_proj.cwiseProduct(tar.m_proj);

    // non overlapping
    bool all_zero = !loop_exist(ref_t_tar, [](scaler_t x){return x!=0;});
    if (all_zero) return 0;

    // ref - tar
    sparse_mat_t ref_m_tar = ref.m_proj - tar.m_proj;
    bool ref_m_tar_neg = loop_exist(ref_m_tar, [](scaler_t x){return x<0;});
    bool ref_m_tar_pos = loop_exist(ref_m_tar, [](scaler_t x){return x>0;});

    // partial overlapping
    if (ref_m_tar_neg && ref_m_tar_pos) {
        return 0;
    }

    // no non-overlapping pixels
    // TODO: all dead is also non-zero?
    bool ref_non_zero = loop_exist(ref.m_proj, [](scaler_t x){return x!=0;});

    if (!ref_m_tar_neg && !ref_m_tar_pos) {
        if (ref_non_zero) {
            return 2;
        }
        return -2;
    }

    // ref > tar
    if (ref_m_tar_pos) {
        return 1;
    }

    return -1;
}


// 1: tar is part of ref
// -1: ref is part of tar
// 2: tar is equal to ref
// 0 ref and tar do not belong to each other
int WireCell::Img::Projection2D::judge_coverage_alt(const Projection2D& ref, const Projection2D& tar) {
    // TODO: implement me!
    return 0;
}