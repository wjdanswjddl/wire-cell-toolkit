/** DepoFluxSplat is an approximate combination DepoTransform and OmnibusSigProc.

    DepoFluxSplat transforms an IDepoSet into an IFrame that represents a "true
    signal" comparable to the output of signal processing.  It does this by
    adding configurable amount of Gaussian smearing to the drifted depos to
    account for the smearing accrued by the convolution/deconvolution cycle of
    sim+sigproc and then bins the resulting distribution onto channels via their
    wires.

    DepoFluxSplat operates on the context of a single anode and will write
    either a sparse or dense frame.  When the frame is sparse, each depo results
    in a family of traces that will in general overlap in tick and channel with
    the family of traces from nearby depos.

    Differences between DepoFluxSplat and DepoSplat:
    - DepoFluxSplat is general to all detectors (no hard-wired magic numbers)
    - DepoFluxSplat consumes IDepoSet instead of IDepo

    Differences between DepoFluxSplat and DepoFluxWriter (from larwirecell)
    - DepoFluxSplat is "pure" Wire-Cell, no interace with art::Event.
    - DepoFluxSplat outputs IFrame instead of SimChannel
    - DepoFluxSplat operates in the context of a single AnodePlane.
    - DepoFluxSplat drops the misguided handling of "energy".

    DepoFluxSplat internals are largely copy-pasted from DepoFluxWriter.  It is
    hoped that DepoFluxWriter will be refactored to replace its internals with
    instances of DepoFluxSplat.

*/
#ifndef WIRECELLGEN_DEPOFLUXSPLAT
#define WIRECELLGEN_DEPOFLUXSPLAT

#include "WireCellAux/Logger.h"
#include "WireCellIface/IDepoFramer.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IAnodeFace.h"

namespace WireCell::Gen {

    class DepoFluxSplat : public Aux::Logger,
                          public IDepoFramer,
                          public IConfigurable
    {
      public:
        DepoFluxSplat();
        ~DepoFluxSplat();

        virtual bool operator()(const input_pointer& in, output_pointer& out);

        virtual void configure(const WireCell::Configuration& cfg);
        virtual WireCell::Configuration default_configuration() const;

      private:

        /// Configuration:
        ///
        /// anodes - a string or array of string naming IAnodePlane
        /// instances.
        ///
        std::vector<WireCell::IAnodePlane::pointer> m_anodes;

        /// field_response - name of an IFieldResponse.
        double m_speed{0}, m_origin{0};

        /// tick - the sample time over which to integrate depo flux
        /// into time bins.
        ///
        /// window_start - the start of an acceptance window in NOMINAL
        /// TIME.
        ///
        /// window_duration - the duration of the acceptance window.
        WireCell::Binning m_tbins;

        /// sparse - bool, if true save a sparse IFrame, else dense.  Default is
        /// false.
        bool m_sparse{false};

        /// nsigma - number of sigma at which to truncate a depo Gaussian
        double m_nsigma{3.0};

        /// referenced_time - An arbitrary time that is SUBTRACTED from the
        /// NOMINAL TIME of the window start when setting the output frame time.
        double m_reftime{0};

        /// time_offsets - An arbitrary time that is ADDED to NOMINAL TIMES for
        /// each plane.  If set, this offset is reflected in the "tbin" value of
        /// traces for a given plane.
        std::vector<int> m_tick_offsets;

        /// smear_tran - Extra smearing applied to the depo in the
        /// transverse direction of each plane.  This given in units of
        /// pitch (ie, smear_tran=2.0 would additionally smear over 2
        /// pitch distances) and it is added in quadrature with the
        /// Gaussian sigma of the depo.  If zero (default) no extra
        /// smearing is applied.
        std::vector<double> m_smear_tran{0};

        /// smear_long - Extra smearing applied to the depo in the
        /// longitudinal direction.  This given in units of tick (ie,
        /// smear_long=2.0 would additionally smear across two ticks)
        /// and added in quadrature with the Gaussian sigma of the
        /// depo.  If zero (default) no extra smearing is applied.
        double m_smear_long{0};


        WireCell::IAnodeFace::pointer find_face(const WireCell::IDepo::pointer& depo);

        size_t m_count{0};

    };

}

#endif 
