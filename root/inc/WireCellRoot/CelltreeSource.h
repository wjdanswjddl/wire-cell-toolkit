/* Celltree input for ( noise filter + signal processing ) */
#ifndef WIRECELLROOT_CELLTREEFILESOURCE
#define WIRECELLROOT_CELLTREEFILESOURCE

#include "WireCellIface/IFrameSource.h"
#include "WireCellIface/IConfigurable.h"

#include <unordered_map>

namespace WireCell {
    namespace Root {
        class CelltreeSource : public IFrameSource, public IConfigurable {
           public:
            CelltreeSource();
            virtual ~CelltreeSource();

            virtual bool operator()(IFrame::pointer& out);

            virtual WireCell::Configuration default_configuration() const;
            virtual void configure(const WireCell::Configuration& config);

           private:
            Configuration m_cfg;
            int m_calls;
            bool read_traces(ITrace::vector& all_traces,
                               std::unordered_map<IFrame::tag_t, IFrame::trace_list_t>& tagged_traces, std::string& fname,
                               const std::string& br_name, const std::string& frametag, const unsigned int entry, const int time_scale) const;
            bool read_cmm(WireCell::Waveform::ChannelMaskMap &cmm, std::string& fname, const unsigned int entry) const;
        };
    }  // namespace Root
}  // namespace WireCell

#endif
