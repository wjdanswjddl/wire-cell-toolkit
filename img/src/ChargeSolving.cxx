// This reimplements the ideas from:
// https://github.com/BNLIF/wire-cell-2dtoy/blob/master/src/ChargeSolving.cxx
// and
// https://github.com/BNLIF/wire-cell-2dtoy/blob/master/src/MatrixSolver.cxx

#include "WireCellImg/ChargeSolving.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"
#include <iterator>

WIRECELL_FACTORY(ChargeSolving, WireCell::Img::ChargeSolving,
                 WireCell::INamed,
                 WireCell::IClusterFilter, WireCell::IConfigurable)

using namespace WireCell;

Img::ChargeSolving::ChargeSolving()
    : Aux::Logger("ChargeSolving", "img")
{
}

Img::ChargeSolving::~ChargeSolving() {}

void Img::ChargeSolving::configure(const WireCell::Configuration& cfg)
{
    m_minimum_measure = get(cfg, "minimum_measure", m_minimum_measure);
    m_lasso_tolerance = get(cfg, "lasso_tolerance", m_lasso_tolerance);
    m_lasso_minnorm = get(cfg, "lasso_minnorm", m_lasso_minnorm);
}

WireCell::Configuration Img::ChargeSolving::default_configuration() const
{
    WireCell::Configuration cfg;
    cfg["minimum_measure"] = m_minimum_measure;
    cfg["lasso_tolerance"] = m_lasso_tolerance;
    cfg["lasso_minnorm"] = m_lasso_minnorm;
    return cfg;
}

// Return a vector of connected b-m (only) graphs from the slice.
static std::vector<cluster_indexed_graph_t>
slice_graph_groups(cluster_indexed_graph_t& grind, ISlice::pointer islice)
{
    // Get the blobs in the slice.
    std::vector<cluster_node_t> bms;
    grind.neighbors(back_inserter(bms), cluster_node_t(islice),
                    [](const cluster_node_t& cn) {
                        return cn.code() == 'b';
                    });

    // find the m's neighbors to these b's
    std::unordered_set<cluster_node_t> mnodes;
    for (auto& bnode : bms) {
        grind.neighbors(std::inserter(mnodes, mnodes.end()), bnode, 
                        [](const cluster_node_t& cn) {
                            return cn.code() == 'm';
                        });
    }

    // Collect b's and m's
    bms.insert(bms.end(), mnodes.begin(), mnodes.end());

    // Induce a subgraph with just the b's and m's from the slice.
    auto sg = grind.induced_subgraph(bms.begin(), bms.end());

    // Partition b's and m's into connected sub sets.  This is a map
    // from group number to vector.  Group number doesn't have any
    // particular meaning.
    auto groups = sg.groups();

    // Convert each group of vertices into a subgraph
    std::vector<cluster_indexed_graph_t> ret;
    for (auto& group : groups) {
        ret.push_back(sg.induced_subgraph(group.second.begin(), group.second.end()));
    }
    return ret;
}


// Return an inherent ordering value for a blob.  It does not matter
// what ordering results but it must be stable on re-running.  So, no
// pointers.  The Blob ident is set at blob creation so is inherent
// and stable in that scope if the same slice is re-tiled.
int blob_ordering(const IBlob::pointer& blob)
{
    return blob->ident();
}

    
// Return an inherent ordering value for a measure.  It does not
// matter what ordering results but it must be stable on re-running.
// We return the smallest channel ident in the measure on the
// assumption that each measure has a unique set of channels w/in
// its slice.
static int measure_ordering(const IChannel::shared_vector& m)
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

// Return the sum of values on channels in the meaure
static ISlice::value_t measure_sum(const IChannel::shared_vector& imeas, const ISlice::pointer& islice)
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
    return ISlice::value_t(value);
}




// using ress_vector_t = WireCell::Ress::vector_t;
// using ress_matrix_t = WireCell::Ress::matrix_t;

// We need to associate some things with a "measure"
struct MeasureInfo {
    // Some value on which an arbitrary but stable sort order can be
    // formed.
    int ordering;
    // The measure representation in the indexed graph
    IChannel::shared_vector node;
    // The value+error summed over the measure
    ISlice::value_t value;
};
using minfo_vector_t = std::vector<MeasureInfo>;

// This will return an ordered vector of measure info for
// measures in the graph with passes threshold.  Threshold value
// is taken as a minimum and threshold error as a maximum, but with
// either <= 0, either theshold condition is not applied.
static
minfo_vector_t make_minfos(ISlice::pointer islice,
                           cluster_indexed_graph_t sg,
                           ISlice::value_t threshold)
{
    minfo_vector_t ret;
    for (auto imeas : oftype<IChannel::shared_vector>(sg)) {
        auto valerr = measure_sum(imeas, islice);
        const auto tvalue = threshold.value();
        if (tvalue > 0 and valerr.value() < tvalue) {
            continue;
        }
        const auto terror = threshold.uncertainty();        
        if (tvalue > 0 and valerr.uncertainty() > terror) {
            continue;
        }
        auto ordering = measure_ordering(imeas);
        ret.emplace_back(MeasureInfo{ordering, imeas, valerr});
    }
    std::sort(ret.begin(), ret.end(), [](const MeasureInfo& a,
                                         const MeasureInfo& b) {
        return a.ordering < b.ordering;
    });

    return ret;
}

struct BlobInfo {
    int ordering;
    IBlob::pointer node;
    double weight{0};
};
using binfo_vector_t = std::vector<BlobInfo>;

static
binfo_vector_t make_binfos(ISlice::pointer islice,
                           cluster_indexed_graph_t sg)
{
    binfo_vector_t ret;
    for (auto iblob : oftype<IBlob::pointer>(sg)) {
        auto ordering = blob_ordering(iblob);
        ret.emplace_back(BlobInfo{ordering, iblob});
    }
    
    std::sort(ret.begin(), ret.end(), [](const BlobInfo& a,
                                         const BlobInfo& b) {
        return a.ordering < b.ordering;
    });

    return ret;
}



static
void solve(cluster_indexed_graph_t& full, ISlice::pointer islice,
           cluster_indexed_graph_t sg)
{
    // Get ordered blob info
    auto binfos = make_binfos(islice, sg);

    // Get ordered measure info
    auto minfos = make_minfos(islice, sg, 0.0);


    // get sum for each measure.  value makes vector m and uncertainty
    // makes diagonal matrix C_m.
    

    // get overlap with neighboring blobs as a form of weight.  This
    // is the beta of Lasso.
}

bool Img::ChargeSolving::operator()(const input_pointer& in, output_pointer& out)
{
    if (!in) {
        out = nullptr;
        log->debug("EOS");
        return true;
    }

    cluster_indexed_graph_t grind(in->graph());

    for (auto islice : oftype<ISlice::pointer>(grind)) {
        // For each of connected b-m graphs from the slice
        for (const auto& sg : slice_graph_groups(grind, islice)) {
            solve(grind, islice, sg);

            // update blobs in grind from solved versions
        }
        
    }

    return true;
}
