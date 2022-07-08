#include "WireCellImg/BlobDepoFill.h"

#include "WireCellAux/SliceTools.h"
#include "WireCellAux/DepoTools.h"
#include "WireCellAux/SimpleBlob.h"

#include "WireCellUtil/GraphTools.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/GraphTools.h"
#include "WireCellUtil/NamedFactory.h"


WIRECELL_FACTORY(BlobDepoFill, WireCell::Img::BlobDepoFill,
                 WireCell::INamed,
                 WireCell::IBlobDepoFill, WireCell::IConfigurable)

using namespace WireCell;
using WireCell::Aux::sensitive;
using WireCell::GraphTools::mir;

Img::BlobDepoFill::BlobDepoFill()
    : Aux::Logger("BlobDepoFill", "img")
{
}
Img::BlobDepoFill::~BlobDepoFill()
{
}

WireCell::Configuration Img::BlobDepoFill::default_configuration() const
{
    Configuration cfg;

    // When drifted depos are sent through DepoTransform to form the
    // input to blobs, they gain a time offset due to the "start_time"
    // config parameter.  This provided time offset is added to depo
    // times prior to associating with blobs.
    cfg["time_offset"] = m_toffset;

    // The number of sigma of depo Gaussian to consider.
    cfg["nsigma"] = m_nsigma;
    
    // The nominal drift speed used to convert the longitudinal extent
    // of depos from units [distance] to units of [time].
    cfg["speed"] = m_speed;

    // The plane index to use to use as the "primary" plane.  The
    // depos will be "diced" onto the wires of this plane.
    cfg["pindex"] = m_pindex;

    return cfg;
}


void Img::BlobDepoFill::configure(const WireCell::Configuration& cfg)
{
    m_toffset = get(cfg, "time_offset", m_toffset);
    m_nsigma = get(cfg, "nsigma", m_nsigma);
    m_speed = get(cfg, "speed", m_speed);
    m_pindex = get(cfg, "pindex", m_pindex);

    if (m_speed <= 0.0) {
        THROW(ValueError() << errmsg{"no speed provided"});
    }
}

// A directed graph holding s-d-w nodes with weighted edges giving the
// integral of the Gausian section.  Always add edges in order as:
// add_edge(s,d) and add_edge(d,w)
namespace idweight {
    struct node_t { size_t idx; char code; };
    struct edge_t { double weight; };
    using graph_t = boost::adjacency_list<boost::setS, boost::vecS, boost::directedS,
                                          node_t, edge_t>;
    using vdesc_t = boost::graph_traits<graph_t>::vertex_descriptor;
    using edesc_t = boost::graph_traits<graph_t>::edge_descriptor;
}

// Produce an s-d-w graph with nodes packed in [{s},{d},{w}] order
static
idweight::graph_t
slice_and_dice_depos(const Binning& sbins,       // run over s nodes
                     const IDepo::vector& depos, // run over d nodes
                     const Pimpos& pimpos,       // run over w nodes
                     double speed, // to convert longitudinal extent to time
                     double nsigma, double time_offset)
{
    idweight::graph_t gr;
    
    // Add slice nodes 
    const size_t nsbins = sbins.nbins();
    for (size_t idx = 0; idx < nsbins; ++idx) {
        const auto gidx = boost::add_vertex({idx, 's'}, gr);
        assert(gidx == idx);
    }

    // Add depo nodes
    const size_t ndepos = depos.size();
    for (size_t idx=0; idx<ndepos; ++idx) {
        const auto gidx = boost::add_vertex({idx, 'd'}, gr);
        assert(gidx == idx + nsbins);
    }
    
    // Add wire nodes
    const auto pbins = pimpos.region_binning();
    const size_t nwires = pbins.nbins();
    for (size_t idx = 0; idx < nwires; ++idx) {
        const auto gidx = boost::add_vertex({idx, 'w'}, gr);
        assert(gidx == idx + nsbins + ndepos);
    }

    // Make s-d and d-w edges holding Gaussian integration values.
    for (size_t depo_idx=0; depo_idx<ndepos; ++depo_idx) {
        const auto& idepo = depos[depo_idx];
        const auto ddesc = depo_idx + nsbins;

        // first we slice
        {
            const double tmean = idepo->time() + time_offset;
            const double tsigma = idepo->extent_long()/speed;
            const double tmin = tmean - tsigma*nsigma;
            const double tmax = tmean + tsigma*nsigma;
            const Binning tbins = subset(sbins, tmin, tmax);
            std::vector<double> weights(tbins.nbins());
            gaussian(weights.begin(), tbins, tmean, tsigma);

            for (int it=0; it<tbins.nbins(); ++it) {
                const idweight::vdesc_t sdesc = sbins.bin(tbins.center(it));
                boost::add_edge(sdesc, ddesc, {weights[it]}, gr);
            }
        }

        // then we dice
        {
            // (x,y,z) -> (normal, wire, pitch);
            const auto wpos = pimpos.transform(idepo->pos()); 
            const double wmean = wpos[2]; // pitch
            const double wsigma = idepo->extent_tran();

            auto wbins = subset(pbins, wmean-wsigma*nsigma, wmean+wsigma*nsigma);
            std::vector<double> weights(wbins.nbins(), 0);
            gaussian(weights.begin(), wbins, wmean, wsigma);

            // make edge from each d to w
            int wip = pbins.bin(wbins.center(0));
            for (int wind = 0; wind < wbins.nbins(); ++wind) {
                const idweight::vdesc_t wdesc = wip + wind + nsbins + ndepos;
                boost::add_edge(ddesc, wdesc, {weights[wind]}, gr);
            }
        }
    }

    return gr;
}


bool Img::BlobDepoFill::operator()(const input_tuple_type& intup,
                                   output_pointer& out)
{
    out = nullptr;
    auto iblobset = std::get<0>(intup);
    auto ideposet = std::get<1>(intup);
    if (!iblobset or !ideposet) {
        if (iblobset or ideposet) {
            log->warn("join node did not get synchronized EOS");
        }
        log->debug("EOS at call={}", m_count);
        ++m_count;
        return true;
    }
    
    // The enormouse code below fills this.
    std::unordered_map<IBlob::pointer, std::pair<double, double>> blob_value;

    // must separate blobs into faces and from each face take pimpos
    // from wire plane at pindex.
    std::unordered_map<IAnodeFace::pointer, IBlob::vector> face2blobs;
    for (const auto& iblob : iblobset->blobs()) {
        face2blobs[iblob->face()].push_back(iblob);
        blob_value[iblob] = {0.0, 0.0};
    }

    // Process each face separately
    for (const auto& [iaf, ibv] : face2blobs) {

        auto iwp = iaf->planes()[m_pindex];
        const Pimpos* pimpos = iwp->pimpos();
        const auto& coords = iaf->raygrid();

        // Get slices spanned by this face's blobs
        std::unordered_set<ISlice::pointer> slices;
        for (const auto& iblob : ibv) {
            slices.insert(iblob->slice());
        }

        // Binning spanning the slices' start and span times.  Because
        // of the order that the s-d-w graph below is filled, the
        // sbins indices are also the s-node descriptors.
        const auto sbins = Aux::binning(slices.begin(), slices.end());

        // Get depos in the sensitive area of the current face and
        // precalcualte their central position in the primary wire
        // plane coordinate system.
        IDepo::vector depos;
        std::vector<double> dwcenter;
        for (const auto& maybe : sensitive(*(ideposet->depos()), iaf)) {
            const auto rpos = pimpos->relative(maybe->pos());
            if (rpos[0] < 0) {
                continue;       // depo is behind the face.
            }
            depos.push_back(maybe);
            dwcenter.push_back(pimpos->transform(maybe->pos())[1]);
        }

        // Make the s-d-w graph holding Gaussian integrals.  Note,
        // nodes are indexed in order: [{s},{d},{w}]
        auto gr = slice_and_dice_depos(sbins, depos, *pimpos,
                                       m_speed, m_nsigma, m_toffset);

        // Process blobs on a per-slice basis for better caching
        std::unordered_map<ISlice::pointer, IBlob::vector> slice2blobs;
        for (const auto& iblob: ibv) {
            ISlice::pointer islice = iblob->slice();
            slice2blobs[islice].push_back(iblob);
        }

        // Each slice
        for (const auto& [islice,blobs] : slice2blobs) {
            const auto sdesc = sbins.bin(islice->start() + 0.5*islice->span());

            // Each depo in the slice
            for (auto sdedge : mir(boost::out_edges(sdesc, gr))) {
                const double sd_weight = gr[sdedge].weight;
                const auto ddesc = boost::target(sdedge, gr);
                const auto didx = gr[ddesc].idx;
                const auto& idepo = depos[didx];
                const double depo_center = dwcenter[didx];
                const double depo_min = depo_center - idepo->extent_tran()*m_nsigma;
                const double depo_max = depo_center + idepo->extent_tran()*m_nsigma;
                const double depo_scale = 1.0/(sqrt(2.0)*idepo->extent_tran());

                // Each wire hit by the depo
                for (auto dwedge : mir(boost::out_edges(ddesc, gr))) {
                    const double dw_weight = gr[dwedge].weight;
                    const auto wdesc = boost::target(dwedge, gr);
                    const int wip = gr[wdesc].idx;

                    // Many reloops over the blobs
                    for (const auto& iblob : blobs) {
        
                        // blob bounds
                        const auto& strips = iblob->shape().strips();

                        // wire in plane range for blob
                        const auto& wiprange = strips[2 + m_pindex].bounds;

                        if (wip < wiprange.first or wip >= wiprange.second) {
                            continue; // wire not in blob.
                        }

                        RayGrid::coordinate_t rgc{2+m_pindex, wip};

                        std::vector<double> lo, hi;
                        // fixme: assumes first 2 layers are overall bounds
                        const int nfake = 2;
                        const int nplanes = coords.nlayers() - nfake;
                        for (int irel=1; irel<nplanes; ++irel) {
                            int other_pindex = (m_pindex + irel)%nplanes;
                            const auto owipr = strips[nfake + other_pindex].bounds;

                            RayGrid::coordinate_t orgc1{nfake+other_pindex, owipr.first};
                            RayGrid::coordinate_t orgc2{nfake+other_pindex, owipr.second};

                            auto p1 = coords.ray_crossing(rgc, orgc1);
                            auto p2 = coords.ray_crossing(rgc, orgc2);

                            auto w1 = pimpos->transform(p1)[1];
                            auto w2 = pimpos->transform(p2)[1];
                            if (w2 < w1) std::swap(w1,w2);
                            
                            lo.push_back(w1);
                            hi.push_back(w2);
                        }
                        const double wlo = *std::max_element(lo.begin(), lo.end());
                        const double whi = *std::min_element(hi.begin(), hi.end());

                        const double w1 = std::max(wlo, depo_min);
                        const double w2 = std::min(whi, depo_max);
                        const double e1 = std::erf(depo_scale*(w1 - depo_center));
                        const double e2 = std::erf(depo_scale*(w2 - depo_center));
                        const double w_weight = e2 - e1;
                        const double dq = sd_weight * dw_weight * w_weight * idepo->charge();
                        auto& bv = blob_value[iblob];
                        bv.first += dq;
                        bv.second += 0; // how to estimate?
                    } // over blobs
                } // over wires
            } // over s-d edges
        } // over slices
    } // over faces


    // copy out
    IBlob::vector out_blobs;
    for (const auto& iblob : iblobset->blobs()) {
        const auto& bv = blob_value[iblob];
        out_blobs.push_back(
            std::make_shared<Aux::SimpleBlob>(
                iblob->ident(),
                bv.first,
                bv.second,
                iblob->shape(),
                iblob->slice(),
                iblob->face()
                ));
    }
    out = std::make_shared<Aux::SimpleBlobSet>(iblobset->ident(), iblobset->slice(), out_blobs);
    return true;
}


