#include "WireCellAux/TensorCluster.h"
#include "WireCellAux/TensorDM.h"
#include "WireCellAux/ClusterHelpers.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(TensorCluster, WireCell::Aux::TensorCluster,
                 WireCell::INamed,
                 WireCell::ITensorSetCluster)

using namespace WireCell;
using namespace WireCell::Aux;

TensorCluster::TensorCluster()
  : Aux::Logger("TensorCluster", "aux")
{
}
TensorCluster::~TensorCluster()
{
}


const std::string default_dpre = "clusters/[[:digit:]]+$";

WireCell::Configuration TensorCluster::default_configuration() const
{
    Configuration cfg;
    cfg["dpre"] = default_dpre;
    cfg["anodes"] = Json::arrayValue;
    return cfg;
}


void TensorCluster::configure(const WireCell::Configuration& cfg)
{
    m_dpre = get(cfg, "dpre", default_dpre);

    m_anodes.clear();
    for (const auto& janode : cfg["anodes"]) {
        m_anodes.push_back(Factory::find_tn<IAnodePlane>(janode.asString()));
    }
}


bool TensorCluster::operator()(const input_pointer& its, output_pointer& cluster)
{
    cluster = nullptr;
    if (! its) {
        log->debug("EOS at call={}", m_count++);
        return true;
    }

    std::regex dpre(m_dpre);

    const ITensor::vector& tens = *(its->tensors().get());
    for (const auto& ten : tens) {
        std::string dp = ten->metadata()["datapath"].asString();
        if (std::regex_match(dp, dpre)) {
            try {
                cluster = TensorDM::as_cluster(tens, m_anodes, dp);
            }
            catch (ValueError& err) {
                log->error(err.what());
                log->error("failed to convert cluster at call={} with tensor datapaths:", m_count++);
                for (const auto& ten : tens) {
                    std::string dp = ten->metadata()["datapath"].asString();
                    log->error("\t{}", dp);
                }
                return false;
            }
            break;
        }
    }
    if (!cluster) {
        log->error("no tensor datapath matching {} at call={}. checked:", m_dpre, m_count++);
        for (const auto& ten : tens) {
            std::string dp = ten->metadata()["datapath"].asString();
            log->error("\t{}", dp);
        }
        return false;
    }
    log->debug("call={} output cluster: {}", m_count++, dumps(cluster->graph()));

    return true;
}
