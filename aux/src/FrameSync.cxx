#include "WireCellAux/FrameSync.h"

#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(FrameSync,
                 WireCell::Aux::FrameSync,
                 WireCell::IFrameMerge)


using namespace WireCell;

Aux::FrameSync::FrameSync(const size_t multiplicity)
    : Aux::Logger("FrameSync", "glue")
    , m_multiplicity(multiplicity) { }
Aux::FrameSync::~FrameSync() { }

std::vector<std::string> Aux::FrameSync::input_types()
{
    const std::string tname = std::string(typeid(input_type).name());
    std::vector<std::string> ret(m_multiplicity, tname);
    return ret;
}

bool Aux::FrameSync::operator()(input_queues& iqs, output_queues& oqs)
{
    log->debug("FrameSync: iqs size: {} oqs size: {}", iqs.size(), std::tuple_size<output_queues>::value);

    size_t neos=0, nmin=0;
    int min_ident=0;
    size_t min_index=0;

    const size_t nin = iqs.size();
    for (size_t ind=0; ind<nin; ++ind) {
        auto& iq = iqs[ind];
        if (iq.empty()) {
            continue;
        }
        auto frame = iq.front();
        if (!frame) {
            ++neos;
            continue;
        }
        if (min_ident > frame->ident()) {
            min_ident = frame->ident();
            min_index = ind;
            ++nmin;
        }
    }

    // found min (good)
    if (nmin > 0) {
        auto frame = iqs[min_index].front();
        iqs[min_index].pop_front();
        std::get<0>(oqs).push_back(frame);

        // May have more
        return (*this)(iqs, oqs);
    }

    // push eos
    for (size_t ind=0; ind<nin; ++ind) {
        auto& iq = iqs[ind];
        if (iq.empty()) {
            continue;
        }
        auto frame = iq.front();
        if (!frame) {
            std::get<0>(oqs).push_back(nullptr); // forward EOS
            iq.pop_front();
            return (*this)(iqs, oqs);
        }
    }

    /// FIXEME: check all empty
    return true;
}
