#include "WireCellGen/DepoFluxSplat.h"

#include "WireCellIface/IFieldResponse.h"

#include "WireCellAux/SimpleFrame.h"
#include "WireCellAux/SimpleTrace.h"

#include "WireCellUtil/Range.h"
#include "WireCellUtil/Array.h"
#include "WireCellUtil/Configuration.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"

#include "WireCellGen/GaussianDiffusion.h"

#include <functional>           //  std::plus

WIRECELL_FACTORY(DepoFluxSplat,
                 WireCell::Gen::DepoFluxSplat,
                 WireCell::INamed,
                 WireCell::IConfigurable,
                 WireCell::IDepoFramer)

using namespace WireCell;
using namespace WireCell::Aux;
using WireCell::Range::irange;


Gen::DepoFluxSplat::DepoFluxSplat()
    : Aux::Logger("DepoFluxSplat", "gen")
{
}


Gen::DepoFluxSplat::~DepoFluxSplat()
{
}


WireCell::Configuration Gen::DepoFluxSplat::default_configuration() const
{
    Configuration cfg;

    // Accept array of strings or single string
    cfg["anode"] = Json::arrayValue;
    cfg["field_response"] = Json::stringValue;

    // time binning
    const double default_tick = 0.5 * units::us;
    cfg["tick"] = default_tick;
    cfg["window_start"] = 0;
    cfg["window_duration"] = 8096 * default_tick;

    cfg["sparse"] = m_sparse;

    cfg["nsigma"] = m_nsigma;

    cfg["reference_time"] = 0;
    cfg["time_offsets"] = Json::arrayValue;

    cfg["smear_long"] = 0.0;
    cfg["smear_tran"] = 0.0;

    return cfg;
}

void Gen::DepoFluxSplat::configure(const WireCell::Configuration& cfg)
{
    // For response plane info
    auto jfrname = cfg["field_response"];
    if (! jfrname.isNull()) {
        auto ifr = Factory::find_tn<IFieldResponse>(jfrname.asString());
        const auto& fr = ifr->field_response();
        m_speed = fr.speed;
        m_origin = fr.origin;
    }
    auto jspeed = cfg["drift_speed"];
    if (! jspeed.isNull()) {
        m_speed = jspeed.asDouble();
    }
    auto jorigin = cfg["response_plane"];
    if (! jorigin.isNull()) {
        m_origin = jorigin.asDouble();
    }

    // Anode plane for down-selecting depos.
    std::string anode_tn = cfg["anode"].asString();
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);

    // Acceptance window 
    const double wtick = get(cfg, "tick", 0.5 * units::us);
    const double wstart = cfg["window_start"].asDouble();
    const double wduration = cfg["window_duration"].asDouble();
    const int nwbins = wduration / wtick;
    m_tbins = Binning(nwbins, wstart, wstart + nwbins * wtick);

    // Frame form.
    m_sparse = get(cfg, "sparse", m_sparse);

    // Gaussian cut-off.
    m_nsigma = get(cfg, "nsigma", m_nsigma);

    // Additional smearing.
    m_smear_long = get(cfg, "smear_long", m_smear_long);
    m_smear_tran.clear();
    auto jst = cfg["smear_tran"];
    if (jst.isDouble()) {
        const double s = jst.asDouble();
        m_smear_tran = {s, s, s};
    }
    else if (jst.size() == 3) {
        for (const auto& js : jst) {
            m_smear_tran.push_back(js.asDouble());
        }
    }

    // Arbitrary time subtracted from window_start when setting frame time
    m_reftime = get(cfg, "reference_time", m_reftime);

    // Arbitrary time added to tbin of traces on per plane basis.
    m_tick_offsets.clear();
    m_tick_offsets.resize(3,0);
    auto jto = cfg["time_offsets"];
    if (jto.isArray()) {
        if (jto.size() == 3) {
            for (int ind = 0; ind < 3; ++ind) {
                m_tick_offsets[ind] = jto[ind].asDouble() / wtick;
            }
        }
        else if (!jto.empty()) {
            THROW(ValueError() << errmsg{"DepoFluxSplat: time_offsets must be empty or be a 3-array"});
        }
    }
    log->debug("speed={} mm/us, origin={} mm, tbins: {} {}us ticks: [{},{}]us, reftime={} us, tick offsets=({},{},{})",
               m_speed / (units::mm/units::us), m_origin/units::mm,
               m_tbins.nbins(), m_tbins.binsize()/units::us,
               m_tbins.min()/units::us, m_tbins.max()/units::us,
               m_reftime/units::us,
               m_tick_offsets[0],m_tick_offsets[1],m_tick_offsets[2]);

}

IAnodeFace::pointer Gen::DepoFluxSplat::find_face(const IDepo::pointer& depo)
{
    for (auto face : m_anode->faces()) {
        auto bb = face->sensitive();
        if (bb.inside(depo->pos())) { return face; }
    }
    return nullptr;
}

// Return intersection of two half open ranges.
using intrange_t = std::pair<int,int>;

// Return intersection of half-open ranges r1 and r2.  If r1 is fully before r2,
// return [r2.first,r2.first] and if fully after return [r2.second.r2.second].
static intrange_t intersect(intrange_t const& r1, intrange_t const& r2)
{
    if (r1.second <= r2.first) {
        return std::make_pair(r2.first, r2.first);
    }
    if (r1.first >= r2.second) {
        return std::make_pair(r2.second, r2.second);
    }
    return std::make_pair(std::max(r1.first, r2.first),
                          std::min(r1.second, r2.second));
}

// A base class for sparse vs dense accumulation
struct Accumulator {

    virtual ~Accumulator() {};
    // Accumulate one depo's patch
    virtual void add(int chid, int tbin, const std::vector<float>& charge) = 0;
    virtual IFrame::pointer frame(int ident, double time, double tick) = 0;
    virtual size_t ntraces() const = 0;
};


// Accumulate in a sparse way.
struct SparseAccumulator : public Accumulator {
    ITrace::vector itraces;

    virtual ~SparseAccumulator() {}
    virtual void add(int chid, int tbin, const std::vector<float>& charge) {
        itraces.push_back(std::make_shared<SimpleTrace>(chid,tbin,charge));
    }
    virtual IFrame::pointer frame(int ident, double time, double tick) {
        return std::make_shared<SimpleFrame>(ident, time, itraces, tick);
    }
    virtual size_t ntraces() const { return itraces.size(); }
};

static intrange_t make_intrange(int beg, size_t siz)
{
    return std::make_pair(beg, beg+siz);
}
static intrange_t union_intrange(intrange_t const& r1, intrange_t const& r2)
{
    return std::make_pair(std::min(r1.first, r2.first),
                          std::min(r1.second, r2.second));
}

// Accumulate in a dense way.
struct DenseAccumulator : public Accumulator {
    using SharedSimpleTrace = std::shared_ptr<SimpleTrace>;
    std::map<int, SharedSimpleTrace> traces; // map chid->trace
    virtual void add(int chid, int tbin, const std::vector<float>& charge) {
        auto tp = traces[chid];
        if (!tp) {              // first seen
            traces[chid] = std::make_shared<SimpleTrace>(chid,tbin,charge);
            return;
        }
        const auto& oldcharge = tp->charge();
        auto have = make_intrange(tp->tbin(), oldcharge.size());
        auto want = make_intrange(tbin, charge.size());
        auto need = union_intrange(have, want);
        if (need.first < have.first || need.second > have.second) {
            std::vector<float> newcharge(need.second-need.first, 0);
            std::copy(oldcharge.begin(), oldcharge.end(),
                      newcharge.begin() + have.first-need.first);
            std::transform(charge.begin(), charge.end(),
                           newcharge.begin() + want.first-need.first,
                           newcharge.begin() + want.first-need.first,
                           std::plus<float>());
            tp->charge() = newcharge;
        }
    }

    virtual IFrame::pointer frame(int ident, double time, double tick) {
        ITrace::vector itraces;
        for (const auto& [chid, tp] : traces) {
            itraces.push_back(tp);
        }
        return std::make_shared<SimpleFrame>(ident, time, itraces, tick); 
    }        
    virtual size_t ntraces() const { return traces.size(); }
};



bool Gen::DepoFluxSplat::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("EOS at {}", m_count);
        ++m_count;
        return true;            // EOS
    }

    std::unique_ptr<Accumulator> accum;
    if (m_sparse) {
        accum = std::make_unique<SparseAccumulator>();
    }
    else {
        accum = std::make_unique<DenseAccumulator>();
    }

    size_t ndepos_seen=0;
    size_t ndepos_skipped=0;
    size_t nplanes_skipped=0;
    for (const auto& depo : *in->depos()) {
        if (!depo) {
            ++ndepos_skipped;
            continue;
        }

        auto face = find_face(depo);
        if (!face) {
            ++ndepos_skipped;
            continue;
        }

        ++ndepos_seen;

        // Depo is at response plane.  Find its time at the collection
        // plane assuming it were to continue along a uniform field.
        // After this, all times are nominal up until we add arbitrary
        // time offsets.
        const double nominal_depo_time = depo->time() + m_origin / m_speed;

        // Allow for extra smear in time.
        double sigma_L = depo->extent_long(); // [length]
        if (m_smear_long) {
            const double extra = m_smear_long * m_tbins.binsize() * m_speed;
            sigma_L = sqrt(sigma_L * sigma_L + extra * extra);
        }
        Gen::GausDesc time_desc(nominal_depo_time, sigma_L / m_speed);

        // Check if patch is outside time binning
        {
            double nmin_sigma = time_desc.distance(m_tbins.min());
            double nmax_sigma = time_desc.distance(m_tbins.max());

            double eff_nsigma = depo->extent_long() > 0 ? m_nsigma : 0;
            if (nmin_sigma > eff_nsigma || nmax_sigma < -eff_nsigma) {
                ++ndepos_skipped;
                continue;
            }
        }

        // fixme: wrap in SPDLOG_LOOGGER_DEBUG
        log->debug("depo {}/{}/[{}] nt={} us, t={} us, x={} mm, q={}",
                   depo->id(), ndepos_seen, ndepos_skipped,
                   nominal_depo_time/units::us,
                   depo->time()/units::us, depo->pos().x()/units::mm,
                   depo->charge());

        // Tabulate depo flux for wire regions from each plane
        for (auto plane : face->planes()) {
            int iplane = plane->planeid().index();
            if (iplane < 0) {
                ++nplanes_skipped;
                continue;
            }

            const Pimpos* pimpos = plane->pimpos();
            auto& wires = plane->wires();
            auto wbins = pimpos->region_binning(); // wire binning

            double sigma_T = depo->extent_tran();
            if (m_smear_tran.size()) {
                const double extra = m_smear_tran[iplane] * wbins.binsize();
                sigma_T = sqrt(sigma_T * sigma_T + extra * extra);
            }

            const double center_pitch = pimpos->distance(depo->pos());
            Gen::GausDesc pitch_desc(center_pitch, sigma_T);
            {
                double nmin_sigma = pitch_desc.distance(wbins.min());
                double nmax_sigma = pitch_desc.distance(wbins.max());

                double eff_nsigma = depo->extent_tran() > 0 ? m_nsigma : 0;
                if (nmin_sigma > eff_nsigma || nmax_sigma < -eff_nsigma) {
                    // We are more than "N-sigma" outside the wire plane.
                    break;
                }
            }

            Gen::GaussianDiffusion gd(depo, time_desc, pitch_desc);
            gd.set_sampling(m_tbins, wbins, m_nsigma, 0, 1);

            // Transfer depo's patch to itraces
            const auto patch = gd.patch(); // 2D array

            // double patch_charge = 0;
            // {
            //     const int np = patch.rows();
            //     const int nt = patch.cols();
            //     for (int ip=0; ip<np; ++ip) {
            //         for (int it=0; it<nt; ++it) {
            //             patch_charge += patch(ip,it);
            //         }
            //     }
            // }
            

            // The absolute pitch bin for the first row of the patch array.
            const int pbin0 = gd.poffset_bin();

            // Absolute pitch bin range truncated to fit wire plane.
            const auto p_range = intersect({pbin0, pbin0 + patch.rows()},
                                           {0, wbins.nbins()});
            if (p_range.first == p_range.second) {
                ++nplanes_skipped;
                continue;
            }

            // The absolute tick bin for the first column of the patch array
            const int tbin0 = gd.toffset_bin();

            // Absolute tick bin range truncated to fit time window.
            const auto t_range = intersect({tbin0, tbin0 + patch.cols()},
                                           {0, m_tbins.nbins()});
            if (t_range.first == t_range.second) {
                ++nplanes_skipped;
                continue;
            }
            // fixme: wrap in SPDLOG_LOOGGER_DEBUG
            // log->debug("pln{} tbin in [{},{}], trange=[{},{}], prange=[{},{}] depo.q={} patch.q={}",
            //            iplane, tbin0, tbin0+patch.cols(),
            //            t_range.first, t_range.second,
            //            p_range.first, p_range.second,
            //            depo->charge(), patch_charge
            //     );

            // Iterate over the valid wires covered by the patch
            for (int pbin : irange(p_range)) {
                auto iwire = wires[pbin];
                const int chid = iwire->channel();

                const int ncharges = t_range.second - t_range.first;
                std::vector<float> charge(ncharges);
                
                const int prel = pbin          - pbin0;
                const int trel = t_range.first - tbin0;
                
                // truly cursed
                Eigen::VectorXf::Map(&charge[0], ncharges) = patch.row(prel).segment(trel, charge.size());

                // Differing conventions exist for the sign of the charge of
                // "number of electrons" in the depo.  Force positive signal.
                std::transform(charge.begin(), charge.end(), charge.begin(),
                               static_cast<float (*)(float)>(&std::abs));
                
                accum->add(chid, t_range.first + m_tick_offsets[iplane], charge);
            } // wires
        }     // plane
    }         // depos

    out = accum->frame(in->ident(), m_tbins.min() - m_reftime, m_tbins.binsize());
    log->debug("splat {} ndepos={}/{}/[{}] ntraces={}", 
               out->ident(), ndepos_seen, in->depos()->size(), nplanes_skipped, accum->ntraces());
    ++m_count;
    return true;
}


// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
