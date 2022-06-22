#include "WireCellSio/ClusterFileSink.h"

#include "WireCellAux/ClusterHelpers.h"
#include "WireCellAux/ClusterArrays.h"

#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellAux/FrameTools.h"

// This is found at *compile* time in util/inc/.
#include "WireCellUtil/custard/custard_boost.hpp"

WIRECELL_FACTORY(ClusterFileSink, WireCell::Sio::ClusterFileSink,
                 WireCell::INamed,
                 WireCell::IClusterSink, WireCell::ITerminal,
                 WireCell::IConfigurable)

using namespace WireCell;

Sio::ClusterFileSink::ClusterFileSink()
    : Aux::Logger("ClusterFileSink", "io")
{
}

Sio::ClusterFileSink::~ClusterFileSink()
{
}

void Sio::ClusterFileSink::finalize()
{
    log->debug("closing {} after {} calls", m_outname, m_count);
    m_out.pop();
}

WireCell::Configuration Sio::ClusterFileSink::default_configuration() const
{
    Configuration cfg;
    // output json file.  A "%d" type format code may be included to be resolved by a cluster identifier.
    cfg["outname"] = m_outname;
    cfg["prefix"] = m_prefix;
    cfg["format"] = m_format;

    return cfg;
}

std::string Sio::ClusterFileSink::fqn(const ICluster& cluster, std::string name, std::string ext)
{
    std::stringstream ss;
    ss << m_prefix << "_" << cluster.ident() << "_" << name << "." << ext;
    return ss.str();
}

void Sio::ClusterFileSink::jsonify(const ICluster& cluster)
{
    auto top = Aux::jsonify(cluster);
    std::stringstream topss;
    topss << top;
    auto tops = topss.str();
    m_out << "name " << fqn(cluster, "graph", "json") << "\n"
          << "body " << tops.size() << "\n" << tops.data();
    m_out.flush();
}



void Sio::ClusterFileSink::dotify(const ICluster& cluster)
{
    auto top = Aux::dotify(cluster);
    std::stringstream topss;
    topss << top;
    auto tops = topss.str();
    m_out << "name " << fqn(cluster, "graph", "dot") << "\n"
          << "body " << tops.size() << "\n" << tops.data();
    m_out.flush();
}


void Sio::ClusterFileSink::numpify(const ICluster& cluster)
{
    Aux::ClusterArrays ca(cluster.graph());
    
    auto fn = [&](const std::string& name) {
        return this->fqn(cluster, name, "npy");
    };

    write_numpy(ca.idents(), fn("idents"));
    write_numpy(ca.edges(), fn("edges"));
    write_numpy(ca.node_ranges(), fn("ranges"));
    write_numpy(ca.signals(), fn("signals"));
    write_numpy(ca.slice_durations(), fn("slice_durations"));
    write_numpy(ca.wire_endpoints(), fn("wire_endpoints"));
    write_numpy(ca.wire_addresses(), fn("wire_addresses"));
    write_numpy(ca.blob_shapes(), fn("blob_shapes"));
}

void Sio::ClusterFileSink::configure(const WireCell::Configuration& cfg)
{
    m_outname = get(cfg, "outname", m_outname);
    m_out.clear();
    custard::output_filters(m_out, m_outname);
    if (m_out.empty()) {
        THROW(ValueError() << errmsg{"ClusterFileSink: unsupported outname: " + m_outname});
    }
    m_prefix = get<std::string>(cfg, "prefix", m_prefix);
    m_format = get<std::string>(cfg, "format", m_format);
    if (m_format == "json") {
        m_serializer = [&](const ICluster& cluster){this->jsonify(cluster);};
    }
    else if (m_format == "dot") {
        m_serializer = [&](const ICluster& cluster){this->dotify(cluster);};
    }
    else if (m_format == "numpy") {
        m_serializer = [&](const ICluster& cluster){this->numpify(cluster);};
    }
    else {
        THROW(ValueError() << errmsg{"ClusterFileSink: unsupported format: " + m_format});
    }
}

bool Sio::ClusterFileSink::operator()(const ICluster::pointer& cluster)
{
    if (!cluster) {             // EOS
        log->debug("see EOS at call={}", m_count);
        ++m_count;
        return true;
    }
    
    m_serializer(*cluster);

    ++m_count;
    return true;
}
