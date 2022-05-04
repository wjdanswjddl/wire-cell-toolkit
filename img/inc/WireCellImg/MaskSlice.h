/* Note, this file should not be #include'd into WCT components
 * directly but may be included into tests.

   Uniformly slice a frame by charge summation

   This file provides two variants.

   - MaskSlicer :: a function node which produces an IFrameSlice

   - MaskSlices :: a queuedout node which produces an ISlice

   The two have trace-offs.

   r: keeps context, makes DFP graph simpler, forces monolithic downstream/more memory/node

   s: the opposite

   Notes:

   - Any sample must be different than 0.0 to be considered for
     addition to a slice but no other thresholding is done here (do it
     in some other component).

*/

#ifndef WIRECELLIMG_MASKSLICE
#define WIRECELLIMG_MASKSLICE

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameSlicer.h"
#include "WireCellIface/IFrameSlices.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellAux/Logger.h"

#include <string>
#include <unordered_map>

namespace WireCell {
    namespace Img {

        namespace Data {
            // ISlice class is held temporarily as concrete.
            class Slice;
        }  // namespace Data

        class MaskSliceBase : public Aux::Logger, public IConfigurable {
           public:
            MaskSliceBase();
            virtual ~MaskSliceBase();

            // IConfigurable
            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

           protected:
            typedef std::map<size_t, Data::Slice*> slice_map_t;
            void slice(const IFrame::pointer& in, slice_map_t& sm);

           private:
            IAnodePlane::pointer m_anode;
            int m_tick_span;
            std::string m_tag{"gauss"};
            std::string m_error_tag{"gauss_error"};
            std::vector<int> m_active_planes;
            std::vector<int> m_dummy_planes;
            std::vector<int> m_masked_planes;
            float m_dummy_charge{0.1};
            float m_dummy_error{1e8};
            float m_masked_charge{0.2};
            float m_masked_error{1e8};
            int m_min_tick{0};
            int m_max_tick{9592};
            int m_tmax;
        };

        class MaskSlicer : public MaskSliceBase, public IFrameSlicer {
           public:
            virtual ~MaskSlicer();

            // IFrameSlicer
            bool operator()(const input_pointer& in, output_pointer& out);
        };

        class MaskSlices : public MaskSliceBase, public IFrameSlices {
           public:
            virtual ~MaskSlices();

            // IFrameSlices
            virtual bool operator()(const input_pointer& depo, output_queue& slices);
        };

    }  // namespace Img
}  // namespace WireCell
#endif
