/* Celltree input for ( noise filter + signal processing ) */
#ifndef WIRECELLROOT_CELLTREEFILESOURCE
#define WIRECELLROOT_CELLTREEFILESOURCE

#include "WireCellIface/IFrameSource.h"
#include "WireCellIface/IConfigurable.h"

class TTree;

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
            bool branch2traces(ITrace::vector& all_traces,
                               std::unordered_map<IFrame::tag_t, IFrame::trace_list_t>& tagged_traces, TTree* tree,
                               const std::string& br_name, const std::string& frametag, const unsigned int entry, const int time_scale) const;
        };
    }  // namespace Root
}  // namespace WireCell

#endif
