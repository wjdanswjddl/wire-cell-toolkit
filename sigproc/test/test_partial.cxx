#include "WireCellSigProc/Diagnostics.h"

#include "WireCellAux/DftTools.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/PluginManager.h"

#include "WireCellUtil/Testing.h"

#include <iostream>
#include <string>

// provides vectors "horig" and "hfilt"
#include <vector>
#include "example-partial-rc.h"

using namespace std;

using namespace WireCell;
using namespace WireCell::SigProc;

int main(int argc, char* argv[])
{
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellAux");
    auto idft = Factory::lookup_tn<IDFT>("FftwDFT");

    auto spectrum = Aux::DftTools::fwd_r2c(idft, horig);
    Diagnostics::Partial m_check_partial;
    bool is_partial = m_check_partial(spectrum);
    Assert(is_partial);

    return 0;
}
