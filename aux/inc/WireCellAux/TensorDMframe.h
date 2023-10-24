/**
   Utilities support the tensor data model for IFrame.
 */
#ifndef WIRECELLAUX_TENSORDMFRAME
#define WIRECELLAUX_TENSORDMFRAME

#include "WireCellIface/IFrame.h"
#include "WireCellIface/ITrace.h"
#include "WireCellIface/ITensor.h"

namespace WireCell::Aux::TensorDM {

    /// Frame support


    /** Convert channel masks to JSON rep. */
    Configuration as_config(const Waveform::ChannelMasks& cms);

    /** Convert channel mask map to JSON rep. */
    Configuration as_config(const Waveform::ChannelMaskMap& cmm);

    /** Convert JSON to CMM */
    Waveform::ChannelMaskMap as_cmm(const Configuration& jcmm);


    /// Default waveform sample (non)transform function.
    inline float identity(float x) { return x; }

    /** Convert a single trace to a trace tensor. */
    ITensor::pointer as_trace_tensor(ITrace::pointer trace,
                                     const std::string& datapath, 
                                     bool truncate=false,
                                     std::function<float(float)> transform=identity);

    /** Convert a sequence of traces to a trace tensor. */
    ITensor::pointer as_trace_tensor(const ITrace::vector& traces,
                                     const std::string& datapath,
                                     bool truncate=false,
                                     std::function<float(float)> transform=identity);

    /** Frame tensor mode.
        
    Frames may be mapped to trace tensors in three ways.
    
    - sparse :: Convert each trace to a 1D trace tensor.  This is the
                most lossless
    
    - unified :: Combine all traces into a 2D trace tensor.  This is
                 lossy.  Overlapping trace extent and individual tbin
                 are both lost.  Zero padding is applied.

    - tagged :: Only consider tagged traces, each set of which is
                stored in a 2D trace tensor.  Each are lossy in ways
                common with unified.  In addition, any trace not
                tagged is dropped.
    */
    enum struct FrameTensorMode { sparse, tagged, unified };

    /**
       Convert an IFrame to tensors.

       The tensor representing the frame aggregate will be the first
       element.

       Trace tensors follow in order reflecting total trace ordering.

       Last come trace data tensors.  These are ordered depending on
       mode and trace tag ordering and come in blocks of pcdataset.

       Note, this ordering is only convenience and the definitive
       ordering is set by the attributes traces and tracedata in the
       frame tensor MD.

       The "datapath" specifies the datapath of the frame tensor and
       is used as the root for the datapath of all other tensors.
       Typically, it should be made unique across all calls.  A good
       choice is "frames/<frame-ident>".

       If truncate is true the trace tensors are of type "i2" else
       "f4".  Prior to truncation, the transform is applied to trace
       array values.
    */
    ITensor::vector as_tensors(IFrame::pointer frame,
                               const std::string& datapath,
                               FrameTensorMode mode = FrameTensorMode::sparse,
                               bool truncate=false,
                               std::function<float(float)> transform=identity);

    /**
      Convert an tensors representing frame to IFrame.

      This is the inverse of as_tensors() above but does not require
      the same ordering.

      The datapath specifies the frame tensor which aggregates the
      others.  If empty then the first tensor of type "frame" is
      assumed.

      If transform is given it will be applied to trace samples after
      any inflation to float is performed.  This transform is
      conceptually the inverse of whatever may have been provided to
      as_tensors() above.

    */
    IFrame::pointer as_frame(const ITensor::vector& tens,
                             const std::string& datapath="",
                             std::function<float(float)> transform=identity);
}

#endif
