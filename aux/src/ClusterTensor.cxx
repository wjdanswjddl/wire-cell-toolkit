#include "WireCellAux/ClusterTensor.h"
#include "WireCellAux/ClusterHelpers.h"

#include "WireCellAux/TensorDM.h"

#include "WireCellUtil/String.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(ClusterTensor, WireCell::Aux::ClusterTensor,
                 WireCell::INamed,
                 WireCell::IClusterTensorSet,
                 WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Aux;
using namespace WireCell::Aux::TensorDM;

ClusterTensor::ClusterTensor()
  : Aux::Logger("ClusterTensor", "aux")
{
}
ClusterTensor::~ClusterTensor()
{
}

WireCell::Configuration ClusterTensor::default_configuration() const
{
    Configuration cfg;
    cfg["datapath"] = m_datapath;
    return cfg;
}

void ClusterTensor::configure(const WireCell::Configuration& cfg)
{
    m_datapath = get(cfg, "datapath", m_datapath);
}

bool ClusterTensor::operator()(const input_pointer& cluster, output_pointer& its)
{
    its = nullptr;
    if (! cluster) {
        // eos
        return true;
    }

    log->debug("call={} input cluster: {}", m_count++, dumps(cluster->graph()));

    const int ident = cluster->ident();
    std::string datapath = m_datapath;
    if (datapath.find("%") != std::string::npos) {
        datapath = String::format(datapath, ident);
    }

    auto tens = as_tensors(cluster, datapath);
    its = as_tensorset(tens, ident);
    return true;
}
        
        
