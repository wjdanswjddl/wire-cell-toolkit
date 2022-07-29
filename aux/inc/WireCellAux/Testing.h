/**
   This API provides various utility functions to help make it easier
   to write plugin test programs (eg in <pkg>/test/test_*.cxx ).

   It provides some default-configured objects.

   Functions here MUST NOT be used in real components as they apply a
   fixed configuration.  When writing WCT components, these
   configurations must be left open for the user to satisfy.

 */

#include "WireCellAux/DftTools.h"
#include "WireCellIface/IRandom.h"

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"

#include "WireCellUtil/NamedFactory.h"

#include <string>
#include <vector>

namespace WireCell::Aux::Testing {

    // Load plugins.  If empty, load "core" plugins.
    void load_plugins(std::vector<std::string> list = {});

    // Return default-configured configurable interface.
    template<typename IFACE>
    typename IFACE::pointer get_default(const std::string& typenam,
                                        const std::string& instnam="")
    {
        auto icfg = WireCell::Factory::lookup<WireCell::IConfigurable>(typenam, instnam);
        auto cfg = icfg->default_configuration();
        icfg->configure(cfg);
        return WireCell::Factory::find<IFACE>(typenam, instnam);
    }

    // Return FftwDFT 
    IDFT::pointer get_dft();

    // Return named Random
    IRandom::pointer get_random(const std::string& name="default");

    // A number of named detectors are 'known' here.
    const std::vector<std::string>& known_detectors();

    // Return some configured anodes for a known detector.
    IAnodePlane::vector anodes(const std::string detector);

    // Initialize WCT logging with argv[0].
    void loginit(const char* argv0);

}

