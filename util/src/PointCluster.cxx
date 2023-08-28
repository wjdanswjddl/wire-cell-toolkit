#include "WireCellUtil/PointCluster.h"

using namespace WireCell;
using namespace WireCell::PointCloud;


Blob PointCloud::envelope(const PointCloud::Blob& a, const PointCloud::Blob& b)
{
    std::vector<Blob> blobs = {a,b};
    return envelope(blobs.begin(), blobs.end();
}

size_t PointCloud::Cluster::insert(const Blob& blob)
{
    size_t index = m_blobs(blob);
    if (index+1 > m_bpcs.size()) {
        m_bpcs.resize(index+1);
    }
    m_envelope_dirty = true;
    return index;
}


void PointCloud::Cluster::blob_pointcloud(const std::string& name, size_t index, const Dataset& pc)
{
    if (index+1 > m_bpcs.size()) {
        raise<IndexError>("Cluster::blob_pointcloud(): index %d out of range for size %d", index, m_bpcs.size());
    }
    m_bpcs[index][name] = pc;
}

const PointCloud::Blob& PointCloud::Cluster::envelope() const
{
    const size_t nuniq = m_blobs.index().size();
    if (!nuniq) {               // no blobs yet
        return Blob{};
    }

    if (m_envelope_last+1 == nuniq) { // up to date
        return m_envelope;
    }

    auto blobs = m_blobs.invert();
    blobs.push_back(m_envelope);
    m_envelope = envelope(blobs.begin(), blobs.end());
    return m_envelope;
}

void PointCloud::Cluster::remove(size_t index)
{
    // invalidate lazy data
    m_envelope = Blob{};

    // remove from m_blobs and m_bpcs
}
