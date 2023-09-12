#include "WireCellUtil/PointTree.h"

using namespace WireCell;
using namespace WireCell::PointCloud;

#include <boost/container_hash/hash.hpp>
#include <string>


std::size_t Tree::Scope::hash() const
{
    std::size_t h = 0;
    boost::hash_combine(h, name);
    for (const auto& s : attrs) {
        boost::hash_combine(h, s);
    }
    boost::hash_combine(h, depth);
    return h;
}


const Tree::NodeValue::Payload& Tree::NodeValue::payload(const Scope& scope) const
{
    auto pcit = cache.find(scope);
    if (pcit != cache.end()) {
        return pcit->second;
    }

    pointcloud_t pc;
    for (auto nv : node->depth(scope.depth)) {

        // local pc
        const auto& lpc = nv.pcs().at(scope.name); // throws

        if (lpc.store().empty()) {
            continue;
        }
        auto spc = lpc.selection(scope.attrs); // throws?
        Dataset::store_t store;
        for (size_t ind=0; ind<spc.size(); ++ind) {
            store[scope.attrs[ind]] = spc[ind];
        }
        pc.append(Dataset(store));
    }
    auto [entry,_] = cache.emplace(scope, Payload(std::move(pc)));
    return entry->second;
}

void Tree::NodeValue::on_construct(Tree::NodeValue::node_type* node_)
{
    node = node_;
}
bool Tree::NodeValue::on_insert(const std::vector<Tree::NodeValue::node_type*>& path)
{
    return true;
}
bool Tree::NodeValue::on_remove(const std::vector<Tree::NodeValue::node_type*>& path)
{
    return true;
}

