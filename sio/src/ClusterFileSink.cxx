#include "WireCellSio/ClusterFileSink.h"

#include "WireCellAux/ClusterHelpers.h"
#include "WireCellAux/ClusterArrays.h"

#include "WireCellUtil/Stream.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellAux/FrameTools.h"

// This is found at *compile* time in util/inc/.
#include "custard/custard_boost.hpp"

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

static 
void jsonify(Sio::ClusterFileSink::ostream_t& out, const ICluster& cluster, const std::string& prefix)
{
    std::stringstream ss;
    ss << prefix << "_" << cluster.ident() << ".json";

    auto top = Aux::jsonify(cluster);
    std::stringstream topss;
    topss << top;
    auto tops = topss.str();
    out << "name " << ss.str() << "\n"
        << "body " << tops.size() << "\n" << tops.data();
    out.flush();
}

static 
void dotify(Sio::ClusterFileSink::ostream_t& out, const ICluster& cluster, const std::string& prefix)
{
    std::stringstream ss;
    ss << prefix << "_" << cluster.ident() << ".json";

    auto top = Aux::dotify(cluster);
    std::stringstream topss;
    topss << top;
    auto tops = topss.str();
    out << "name " << ss.str() << "\n"
        << "body " << tops.size() << "\n" << tops.data();
    out.flush();
}

static 
void numpify(Sio::ClusterFileSink::ostream_t& out, const ICluster& cluster, const std::string& prefix)
{
    Aux::ClusterArrays ca(cluster.graph());
    
    std::stringstream ss;
    ss << prefix << "_" << cluster.ident() << "_";
    std::string arrpre = ss.str();
    
    Stream::write(out, arrpre + "idents", ca.idents());
    Stream::write(out, arrpre + "edges", ca.edges());
    Stream::write(out, arrpre + "ranges", ca.node_ranges());
    Stream::write(out, arrpre + "signals", ca.signals());
    Stream::write(out, arrpre + "slice_durations", ca.slice_durations());
    Stream::write(out, arrpre + "wire_endpoints", ca.wire_endpoints());
    Stream::write(out, arrpre + "wire_addresses", ca.wire_addresses());
    Stream::write(out, arrpre + "blob_shapes", ca.blob_shapes());
    out.flush();
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
        m_serializer = jsonify;
    }
    else if (m_format == "dot") {
        m_serializer = dotify;
    }
    else if (m_format == "numpy") {
        m_serializer = numpify;
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
    
    m_serializer(m_out, *cluster, m_prefix);

    ++m_count;
    return true;
}
