/** Produce point cloud representation of a set of blobs.

    Points are produced by sampling.  Strategies are given by name
    with the "strategy" configuration parameter which may be a string
    (one strategy) or an array of string (multiple strategies).

    The sampling results in a set of PC arrays consisting of a
    "sampling" subset and an "extra" subset.  The content of the
    "sampling" subset is not subject to configuration while the
    content of the "extra" subset is subject to user configuration.

    The names of the PC arrays are as "<prefix><suffix>" where the
    suffix is fixed by this component and the prefix is provided by
    user configuration with default being the empty string.

    The "sampling" arrays, listed by their suffix, are:
 
    - x_coord :: (double) point X-position in the global coordinate system
    - y_coord :: (double) point Y-position in the global coordinate system
    - z_coord :: (double) point Z-position in the global coordinate system
    - t_coord :: (double) point time

    Additional arrays may be include based on the value of the "extra"
    configuration parameter which may be of type string or array of
    string.  Each string is interpreted as a regex and compared
    against the potential array suffix.  

    - sample_strategy :: (int) the ID of the sampling strategy that produced the point.
    - blob_ident :: (int) the blob ident number.
    - blob_index :: (int) the index of the blob in the input vector.

    - slice_ident :: (int) the slice 
    - slice_start :: (double) the start time of the slice.
    - slice_span :: (double) the time span of the slice.

    - {u,v,w}wire_index :: (int) index of nearest wire in a plane (IWirePlane::wires() order)
    - {u,v,w}pitch_coord :: (double) location of point along the pitch direction.
    - {u,v,w}wire_coord :: (double) location of point along the wire direction.

    - {u,v,w}channel_ident :: (int) the ident number of nearest channel (IChannel::ident()).
    - {u,v,w}channel_attach :: (int) the attachment number of nearest channel (IChannel::index()).

    - {u,v,w}charge_val :: (double) the charge value of nearest channel.
    - {u,v,w}charge_unc :: (double) the charge uncertainty of nearest channel.

 */

#ifndef WIRECELL_IMG_BLOBSAMPLER
#define WIRECELL_IMG_BLOBSAMPLER

#include "WireCellIface/IBlobSampler.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"
#include "WireCellUtil/Binning.h"

#include <regex>

namespace WireCell::Img {

    class BlobSampler : public Aux::Logger, public IBlobSampler, public IConfigurable
    {
      public:

        BlobSampler();
        virtual ~BlobSampler() = default;

        // IBlobSampler
        virtual PointCloud::Dataset sample_blobs(const IBlob::vector& blobs);
        
        // IConfigurable
        virtual void configure(const WireCell::Configuration& cfg);
        virtual WireCell::Configuration default_configuration() const;
        
        struct CommonConfig {

            /** Config: "time_offset".  A time value ADDED to the blob
                times prior to processing.  This may be required to remove
                any artificial "start time" from simulation or otherwise
                properly convert from drift time to drift location.
            */
            double time_offset{0*units::us};

            /** Config: "drift_speed".  The drift speed to assume in order
                to translate a blob time to a spacial position in the
                drift direction.
            */
            double drift_speed{1.6*units::mm/units::us};

            /** Config: "prefix".  Sets the string prepended to names of
             * the arrays in the produced dataset. */
            std::string prefix{""};

            /** Config: "tbins", "tmin" and "tmax" */
            Binning tbinning = Binning(1,0.0,1.0);

            /** Config: "extra".  Match potential extra arrays to
             * include in point cloud. */
            std::vector<std::string> extra = {};
            std::vector<std::regex> extra_re = {};

        };
        CommonConfig m_cc;

        /** Config "strategy".

            A full sampling is performed by applying a pipeline of
            functions each enacting a particular sampling strategy.
            The full result is the union of results from each applied
            strategy.

            The "strategy" configuration parameter value may be
            provided in one of these forms:

            - a string providing the strategy name and indicating the
              default configuration for that strategy be used.

            - an object providing a .name attribute naming the
              strategy.  Any other object attributes will be
              considered for providing per-strategy options.

            - an array of "strategy" configuration paramter values.

            The following strategy names are recognized.  They are
            distinquished in how points are sampled in the transverse
            plane.  They all have common options to determine how
            sampled points are projected along the time/drift
            dimension.
        
            - center :: the single point which is the average of the
              blob corner points.  This is the default.

            - corner :: the blob corner points.
            
            - edge :: the center of blob boundary edges.

            - grid :: uniformly spaced points on a ray grid.

              This accepts the following options:

              - step :: grid spacing given as relative to a grid step.
                (def=1.0).

              - planes :: an array of two plane indices, each in
                {0,1}, which determine the ray grid.

              Note, ray crossing points may be sampled with two
              instances of "grid" each with step=1.0 and with mutually
              unique pairs of plane indices.

            - bounds :: points sampled along the blob boundry edges.

              This accepts the following options:

              - step :: grid spacing given as relative to a grid step.
                (def=1.0).

              Note, first point on each boundary edge is one step away
              from the corner.  If "step" is larger than a boundary
              edge that edge will have no samples from this strategy.
              Include "corner" and/or "edge" strategy to include these
              two classes of points missed by "bounds".

            The points from any of the strategies above are projected
            along the time/drift dimension of the blob according to a
            linear grid given by a binning: (tbins, tmin, tmax).  The
            tmin/tmax are measured relative to the time span of the
            blob.  The default binning is (1, 0.0, 1.0) so that a
            single set of samples are made at the start of the blob.

            Each of the strategies may have a different time binning
            by setting:

            - tbins :: the number of time bins (def=1)

            - tmin :: the minimum relative time (def=0.0)

            - tmax :: the maximum relative time (def=1.0)

            These options may be set on the BlobSampler to change the
            default values for all strategies and may be set on each
            individual strategy.

        */
        struct Sampler;
        std::vector<std::unique_ptr<Sampler>> m_samplers;

        void add_strategy(Configuration cfg);
    };

};

#endif

