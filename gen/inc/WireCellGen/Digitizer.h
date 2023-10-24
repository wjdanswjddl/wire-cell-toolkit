/** Digitizer converts voltage waveforms to integer ADC ones.
 *
 * Resulting waveforms are still in floating-point form and should be
 * round()'ed and truncated to whatever integer representation is
 * wanted by some subsequent node.
 */

#ifndef WIRECELL_DIGITIZER
#define WIRECELL_DIGITIZER

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellUtil/Units.h"
#include "WireCellAux/Logger.h"
#include <deque>

namespace WireCell::Gen {


    class Digitizer : public Aux::Logger,
                      public IFrameFilter, public IConfigurable {
      public:
        Digitizer();
        virtual ~Digitizer();

        virtual void configure(const WireCell::Configuration& config);
        virtual WireCell::Configuration default_configuration() const;

        // Voltage frame goes in, ADC frame comes out.
        virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);

        // implementation method.  Return a "floating point ADC"
        // value for the given voltage assumed to have been lifted
        // to the baseline.
        double digitize(double voltage);

      private:
        /// Config: "anode" - type/name of an IAnodePlane
        IAnodePlane::pointer m_anode;

        /// Config: "resolution" - number of ADC bits
        int m_resolution{12};

        /// Config: "gain" - relative input gain
        double m_gain{1.0};

        /// Config: "fullscale" - two element array of min/max ADC range in units of Voltage
        std::vector<double> m_fullscale{0.0, 2.0 * units::volt};
        /// Config: "baselines" - three element array giving nominal baseline of each plane in units of Voltage
        std::vector<double> m_baselines{900 * units::mV, 900 * units::mV, 200 * units::mV};
        /// Config: "frame_tag" - string, if given, a frame tag to apply to produced frames.
        std::string m_frame_tag{""};
        size_t m_count{0};
        /// Config: "round" - bool.  If bool true, round to nearest integer.  Else floor (default).
        bool m_round{false};  // apply round to FP ADC values
    };

}  // namespace WireCell::Gen
#endif
