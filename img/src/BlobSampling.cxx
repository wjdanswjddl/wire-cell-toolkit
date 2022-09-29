#include "WireCellImg/BlobSampling.h"

#include "WireCellUtil/NamedFactory.h"

#include "WireCellUtil/IndexedGraph.h"
#include "WireCellAux/TensorTools.h"

WIRECELL_FACTORY(BlobSampling, WireCell::Img::BlobSampling,
                 WireCell::INamed,
                 WireCell::IBlobSampling,
                 WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;
using namespace WireCell::Aux;

BlobSampling::BlobSampling()
    : Aux::Logger("BlobSampling", "img")
{
}


BlobSampling::~BlobSampling()
{
}


void BlobSampling::configure(const WireCell::Configuration& cfg)
{
    std::string tn = get<std::string>(cfg, "sampler", "BlobSampler");
    m_sampler = Factory::find_tn<IBlobSampler>(tn);
}


WireCell::Configuration BlobSampling::default_configuration() const
{
    Configuration cfg;
    cfg["sampler"] = "BlobSampler";
    return cfg;
}


bool BlobSampling::operator()(const input_pointer& blobset, output_pointer& tensorset)
{
    tensorset = nullptr;
    if (!blobset) {
        log->debug("EOS at call {}", m_count++);
        return true;
    }

    auto blobs = blobset->blobs();
    auto ds = m_sampler->sample_blobs(blobs);
    tensorset = as_itensorset(ds);
    log->debug("sampled {} blobs making {} points at call {}",
               blobs.size(), ds.size_major(), m_count++);
    return true;
}

        

