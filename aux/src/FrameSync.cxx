#include "WireCellAux/FrameSync.h"

#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(FrameSync,
                 WireCell::Aux::FrameSync,
                 WireCell::IFrameMerge)


using namespace WireCell;

Aux::FrameSync::FrameSync(const size_t multiplicity) : m_multiplicity(multiplicity) { }
Aux::FrameSync::~FrameSync() { }

std::vector<std::string> Aux::FrameSync::input_types()
{
    const std::string tname = std::string(typeid(input_type).name());
    std::vector<std::string> ret(m_multiplicity, tname);
    return ret;
}

bool Aux::FrameSync::operator()(input_queues& iqs, output_queues& oqs)
{
    size_t neos=0, nmin=0;
    int min_ident=0;
    size_t min_index=0;

    const size_t nin = iqs.size();
    for (size_t ind=0; ind<nin; ++ind) {
        auto& iq = iqs[0];
        if (iq.empty()) {
            return true;
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

    if (neos == iqs.size()) {   // we have EOS sync
        for (auto& iq : iqs) {
            iq.pop_front();     // pop all EOS
        }
        std::get<0>(oqs).push_back(nullptr); // forward EOS

        // May have more behind the EOS
        return (*this)(iqs, oqs);
    }
        
    if (neos+nmin == nin) {     // fully populated
        auto frame = iqs[min_index].front();
        iqs[min_index].pop_front();
        std::get<0>(oqs).push_back(frame);

        // May have more
        return (*this)(iqs, oqs);
    }

    // If we reach here then we do not have fully populated input
    // streams and must wait for more.
    
    return true;
}
