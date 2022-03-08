/* Note, this file should not be #include'd into WCT components
 * directly but may be included into tests.

   Uniformly slice a frame by charge summation

   This file provides two variants.

   - SumSlicer :: a function node which produces an IFrameSlice

   - SumSlices :: a queuedout node which produces an ISlice

   The two have trace-offs.

   r: keeps context, makes DFP graph simpler, forces monolithic downstream/more memory/node

   s: the opposite

   Notes:

   - Any sample must be different than 0.0 to be considered for
     addition to a slice but no other thresholding is done here (do it
     in some other component).

*/

#ifndef WIRECELLIMG_SUMSLICE
#define WIRECELLIMG_SUMSLICE

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameSlicer.h"
#include "WireCellIface/IFrameSlices.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellAux/Logger.h"

#include <string>

namespace WireCell {
    namespace Img {

        namespace Data {
            // ISlice class is held temporarily as concrete.
            class Slice;
        }  // namespace Data

        class SumSliceBase : public Aux::Logger, public IConfigurable {
           public:
            SumSliceBase();
            virtual ~SumSliceBase();

            // IConfigurable
            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

           protected:
            typedef std::map<size_t, Data::Slice*> slice_map_t;
            void slice(const IFrame::pointer& in, slice_map_t& sm);
            ISlice::pointer make_empty(const IFrame::pointer& inframe);

            // If true, the subclasses will add an EOS at end of
            // slices from one input frame.  See also pad_empty.
            //
            // Note, if slice-level EOS is enabled then it is up to
            // the graph designer to assure something "eats up" the
            // extra EOS.
            //
            // As an alternative, consider leaving this false and rely
            // on a downstream node to re-sync by watching the ID of
            // the frame associated with each slice to change.  Again,
            // see pad_empty.
            bool m_slice_eos{false};

            // If true, then a frame which produces no nominal slices
            // will instead produce a single empty slice.
            bool m_pad_empty{true};

           private:
            IAnodePlane::pointer m_anode;
            int m_tick_span;
            std::string m_tag;
        };

        class SumSlicer : public SumSliceBase, public IFrameSlicer {
           public:
            virtual ~SumSlicer();

            // IFrameSlicer
            bool operator()(const input_pointer& in, output_pointer& out);
        };

        class SumSlices : public SumSliceBase, public IFrameSlices {
           public:
            virtual ~SumSlices();

            // IFrameSlices
            virtual bool operator()(const input_pointer& depo, output_queue& slices);
        };

    }  // namespace Img
}  // namespace WireCell
#endif
