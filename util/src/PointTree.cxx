#include "WireCellUtil/PointTree.h"

using namespace WireCell;
using namespace WireCell::PointCloud;

#include <boost/container_hash/hash.hpp>
#include <string>


std::size_t NodeValue::Scope::hash()
{
    std::size_t h = 0;
    boost::hash_combine(h, name);
    for (const auto& s : attrs) {
        boost::hash_combine(h, s);
    }
    boost::hash_combine(h, depth);
    return h;
}


const NodeValue::pointcloud_t& NodeValue::payload(const Scope& scope) const
{
    auto it = cache.find(scope);
    if (pcit != cache.end()) {
        return pcit.second;
    }

    pointcloud_t pc;
    for (auto nit : node->depth(depth)) {

        // local pc
        const auto& lpc = nit->at(name); // throws

        if (not_empty(lpc)) {
            auto spc = lpc.select_attributes(attrs); // throws?
            pc.append(spc);
        }
    }
    auto got = cache.emplace(scope, Payload(std::move(pc)));
    return *(got.first);
}

void NodeValue::on_construct(NodeValue::node_type* node_)
{
    node = node_;
}
bool NodeValue::on_insert(const std::vector<NodeValue::node_type*>& path);
bool NodeValue::on_remove(const std::vector<NodeValue::node_type*>& path);
