
#include "WireCellIface/ICluster.h"


WireCell::ICluster::~ICluster() {}

const std::string WireCell::cluster_node_t::known_codes = "cwbsm";

size_t WireCell::cluster_node_t::code_index(char code)
{
    size_t ret = 0;
    for (const auto c : known_codes) {
        if (c == code) {
            return ret;
        }
        ++ret;
    }
    return ret;
}

