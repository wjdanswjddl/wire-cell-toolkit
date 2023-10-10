#include "WireCellAux/Testing.h"

#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IConfigurable.h"

#include <algorithm>            // find

using namespace WireCell;
using namespace WireCell::Aux;

void Testing::load_plugins(std::vector<std::string> list)
{
    PluginManager& pm = PluginManager::instance();

    if (list.empty()) {
        list = {"WireCellAux", "WireCellGen", "WireCellSigProc", "WireCellPgraph", "WireCellImg", "WireCellSio", "WireCellApps"};
    }
    for (const auto& one : list) {
        pm.add(one);
    }
}

WireCell::IDFT::pointer Testing::get_dft()
{
    load_plugins({"WireCellAux"});
    return Factory::lookup<IDFT>("FftwDFT"); // not configurable
}


WireCell::IRandom::pointer Testing::get_random(const std::string& name)
{
    using namespace WireCell;
    load_plugins({"WireCellGen"}); // fixme: should move Random to Aux.
    auto rngcfg = Factory::lookup<IConfigurable>("Random", name);
    auto cfg = rngcfg->default_configuration();
    cfg["generator"] = name;    // default or twister
    rngcfg->configure(cfg);
    return Factory::find<IRandom>("Random", name);
}


static std::vector<std::string> knowndets = {"uboone", "pdsp"};
const std::vector<std::string>& Testing::known_detectors()
{
    return knowndets;
}

static void assert_known_det(const std::string& det)
{
    if (std::find(knowndets.begin(), knowndets.end(), det) == knowndets.end()) {
        THROW(ValueError() << errmsg{"unknown detector: " + det});
    }
}

IAnodePlane::vector Testing::anodes(std::string detector)
{
    assert_known_det(detector);
    load_plugins({"WireCellGen", "WireCellSigProc"});
    IAnodePlane::vector anodes;
    {
        int nanodes = 1;
        // Note: these files must be located via WIRECELL_PATH
        std::string ws_fname = "microboone-celltree-wires-v2.1.json.bz2";
        std::string fr_fname = "ub-10-half.json.bz2";
        if (detector == "uboone") {
            ws_fname = "microboone-celltree-wires-v2.1.json.bz2";
            fr_fname = "ub-10-half.json.bz2";
        }
        if (detector == "pdsp") {
            ws_fname = "protodune-wires-larsoft-v4.json.bz2";
            fr_fname = "garfield-1d-3planes-21wires-6impacts-dune-v1.json.bz2";
            nanodes = 6;
        }

        const std::string fr_tn = "FieldResponse";
        const std::string ws_tn = "WireSchemaFile";

        {
            auto icfg = Factory::lookup<IConfigurable>(fr_tn);
            auto cfg = icfg->default_configuration();
            cfg["filename"] = fr_fname;
            icfg->configure(cfg);
        }
        {
            auto icfg = Factory::lookup<IConfigurable>(ws_tn);
            auto cfg = icfg->default_configuration();
            cfg["filename"] = ws_fname;
            icfg->configure(cfg);
        }

        for (int ianode = 0; ianode < nanodes; ++ianode) {
            std::string tn = String::format("AnodePlane:%d", ianode);
            auto icfg = Factory::lookup_tn<IConfigurable>(tn);
            auto cfg = icfg->default_configuration();
            cfg["ident"] = ianode;
            cfg["wire_schema"] = ws_tn;
            cfg["faces"][0]["response"] = 10 * units::cm - 6 * units::mm;
            cfg["faces"][0]["cathode"] = 2.5604 * units::m;
            icfg->configure(cfg);
            anodes.push_back(Factory::find_tn<IAnodePlane>(tn));
        }
    }
    return anodes;
}


// void Testing::loginit(const char* argv0)
// {
//     std::string name = argv0;
//     name += ".log";
//     Log::add_stderr(true, "trace");
//     Log::add_file(name, "trace");
//     Log::set_level("trace");
//     Log::set_pattern("[%H:%M:%S.%03e] %L [%^%=8n%$] %v");
// }
