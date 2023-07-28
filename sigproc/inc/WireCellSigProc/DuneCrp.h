/** Functions and classes for DUNE CRP noise filter
 *  Modified from Protodune.h
 */

#ifndef WIRECELLSIGPROC_DUNECRP
#define WIRECELLSIGPROC_DUNECRP

#include "WireCellSigProc/Diagnostics.h"

#include "WireCellIface/IChannelFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IChannelNoiseDatabase.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IDFT.h"

#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Bits.h"

namespace WireCell {
    namespace SigProc {
        namespace DuneCrp {

            class OneChannelNoise : public WireCell::IChannelFilter, public WireCell::IConfigurable {
               public:
                OneChannelNoise(const std::string& anode_tn = "AnodePlane",
                                const std::string& noisedb = "OmniChannelNoiseDB");
                virtual ~OneChannelNoise();

                // IChannelFilter interface

                /** Filter in place the signal `sig` from given `channel`. */
                virtual WireCell::Waveform::ChannelMaskMap apply(int channel, signal_t& sig) const;

                /** Filter in place a group of signals together. */
                virtual WireCell::Waveform::ChannelMaskMap apply(channel_signals_t& chansig) const;

                void configure(const WireCell::Configuration& config);
                WireCell::Configuration default_configuration() const;

               private:
                std::string m_anode_tn, m_noisedb_tn;
                Diagnostics::Partial m_check_partial;  // at least need to expose them to configuration
                IAnodePlane::pointer m_anode;
                IChannelNoiseDatabase::pointer m_noisedb;
                IDFT::pointer m_dft;
            };

        }  // namespace DuneCrp

    }  // namespace SigProc

}  // namespace WireCell

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
