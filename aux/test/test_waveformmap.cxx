#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IWaveformMap.h"

#include <iostream>
#include <vector>
#include <unordered_set>

using namespace WireCell;

int main(int argc, char* argv[])
{
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellGen");
    {
        auto icfg = Factory::lookup_tn<IConfigurable>("WaveformMap");
        auto cfg = icfg->default_configuration();
        cfg["filename"] = "microboone-charge-error.json.bz2";
        icfg->configure(cfg);
    }
    auto wfm = Factory::find_tn<IWaveformMap>("WaveformMap");
    assert(wfm);

    int errors = 0;
    auto names = wfm->waveform_names();
    if (names.empty()) {
        std::cerr << "test_waveformmap got no waveforms.\n"
                  << "run in environment with microboone-charge-error.json.bz2 in WIRECELL_PATH\n";
        return 0;
    }

    assert(names.size());
    for (auto name: names)  {
        auto wf = wfm->waveform_lookup(name);
        if (!wf) {
            std::cerr << name << ": no such waveform\n";
            ++errors;
            continue;
        }
        auto seq = wf->waveform_samples();
        std::cerr << name << ": t0=" << wf->waveform_start ()
                  << " dt=" << wf->waveform_period()
                  << " n=" << seq.size()
                  << std::endl;
    }

    return errors;
}
