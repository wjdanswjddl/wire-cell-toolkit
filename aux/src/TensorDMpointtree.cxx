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
        ret.emplace(name, ds);
    }
    return ret;
}


ITensor::vector WireCell::Aux::TensorDM::as_tensors(
    const WireCell::PointCloud::Tree::Points::node_t& root,
    const std::string& datapath,
    const std::string& parent_datapath)
{
    // Metadata for "this" node, returned with first tensor
    Configuration md;
    md["datapath"] = datapath;
    md["datatype"] = "pctreenode";
    // datapath to parent tensor
    md["parent"] = parent_datapath;
    // list of datapaths to children tensors
    md["children"] = Json::arrayValue;
    // datapath to pcnamedset holding any local PCs
    md["pointclouds"] = "";

    ITensor::vector ret;
    ret.push_back(nullptr);     // fill in below

    const auto& lpcs = root.value.local_pcs();
    if (lpcs.size()) {
        auto tens = pcnamedset_as_tensors(lpcs.begin(), lpcs.end(),
                                          datapath + "/pointclouds");
        md["pointclouds"] = tens[0]->metadata()["datapath"];
        ret.insert(ret.end(), tens.begin(), tens.end());
    }

    size_t nchild = 0;
    for (const auto& node : root.child_nodes()) {
        std::string childpath = datapath + "/" + std::to_string(nchild);
        ++nchild;
        auto tens = as_tensors(node, childpath, datapath); // recur
        md["children"].append(tens[0]->metadata()["datapath"]);

        ret.insert(ret.end(), tens.begin(), tens.end());
    }

    ret[0] = std::make_shared<SimpleTensor>(md);
    return ret;
}

std::unique_ptr<WireCell::PointCloud::Tree::Points::node_t>
WireCell::Aux::TensorDM::as_pctree(const ITensor::vector& tens,
                                   const std::string& datapath)
{
    using WireCell::PointCloud::Tree::Points;

    std::unordered_map<std::string, Points::node_t*> nodes_by_datapath;
    Points::node_t::owned_ptr root;

    std::function<void(const std::string& dpath)> dochildren;
    dochildren = [&](const std::string& dpath) -> void {


        auto top = top_tensor(tens, "pctreenode", dpath);
        auto const& md = top->metadata();        

        named_pointclouds_t pcns;
        if (! md["pointclouds"] ) {
            pcns = as_pcnamedset(tens, md["pointclouds"].asString());
        }

        std::string ppath = md["parent"].asString();
        if ( ppath.empty() ) {
            if (root) {
                raise<ValueError>("more than one root in pctree encountered");
            }
            root = std::make_unique<Points::node_t>(Points(pcns));
            nodes_by_datapath[dpath] = root.get();
        }
        else {

            auto* parent = nodes_by_datapath[ppath];
            if (!parent) {
                raise<ValueError>("failed to find parent for \"%s\"", ppath);
            }
            auto* node = parent->insert(Points(pcns));
            nodes_by_datapath[dpath] = node;
        }

        for (auto const& jcpath : md["children"]) {
            dochildren(jcpath.asString());
        }
    };
    dochildren(datapath);
        
    return root;
}
