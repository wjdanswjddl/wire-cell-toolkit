#include "WireCellAux/TensorDM.h"
#include "WireCellAux/SimpleTensor.h"


using namespace WireCell;
using namespace WireCell::Aux;
using namespace WireCell::Aux::TensorDM;

WireCell::Aux::TensorDM::named_pointclouds_t
WireCell::Aux::TensorDM::as_pcnamedset(const ITensor::vector& tens, const std::string& datapath)
{
    named_pointclouds_t ret;
    auto top = top_tensor(tens, "pcnamedset", datapath);

    const auto& md = top->metadata();
    auto items = md["items"];
    for (const auto& name : items.getMemberNames()) {
        const auto path = items[name].asString();

        auto ds = as_dataset(tens, path);
        ret.emplace_back(name, ds);
    }
    return ret;
}


ITensor::vector WireCell::Aux::TensorDM::as_tensors(const WireCell::PointCloud::Tree::Points::node_t& node,
                                                    const std::string& datapath)
{
    ITensor::vector ret;

    // 1. walk the tree
    // 2. calculate the tree path from root to node
    // 3. form datapath from tree path
    // 4. convert local PCs to a pcnamedset and convert to tensors
    // 5. save paths to a pctree tensor

    return ret;
}

std::unique_ptr<WireCell::PointCloud::Tree::Points::node_t>
WireCell::Aux::TensorDM::as_pctree(const ITensor::vector& tens,
                                   const std::string& datapath)
{
    return nullptr;
}
