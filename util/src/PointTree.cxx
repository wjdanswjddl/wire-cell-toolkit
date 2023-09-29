#include "WireCellUtil/PointTree.h"
#include "WireCellUtil/Logging.h"
#include <boost/container_hash/hash.hpp>

#include <algorithm>
#include <vector>
#include <string>

using spdlog::debug;
using namespace WireCell::PointCloud;

std::size_t Tree::Scope::hash() const
{
    std::size_t h = 0;
    boost::hash_combine(h, pcname);
    boost::hash_combine(h, depth);
    for (const auto& c : coords) {
        boost::hash_combine(h, c);
    }
    return h;
}

bool Tree::Scope::operator==(const Tree::Scope& other) const
{
    if (depth != other.depth) return false;
    if (pcname != other.pcname) return false;
    if (coords.size() != other.coords.size()) return false;
    for (size_t ind = 0; ind<coords.size(); ++ind) {
        if (coords[ind] != other.coords[ind]) return false;
    }
    return true;
}
bool Tree::Scope::operator!=(const Tree::Scope& other) const
{
    if (*this == other) return false;
    return true;
}

std::ostream& WireCell::PointCloud::Tree::operator<<(std::ostream& o, WireCell::PointCloud::Tree::Scope const& s)
{
    o << "<Scope \"" << s.pcname << "\" L" << s.depth;
    std::string comma = " ";
    for (const auto& cn : s.coords) {
        o << comma << cn;
        comma = ",";
    }
    o << ">";
    return o;
}


//
//  Points
//

void Tree::Points::on_construct(Tree::Points::node_t* node)
{
    m_node = node;
}



// Return true one node at path_length relative to us matches the scope.
static
bool in_scope(const Tree::Scope& scope, const Tree::Points::node_t* node, size_t path_length)
{
    // path must be in our the scope depth
    // path length is 0 for me, 1 for children, 2 for ...
    // scope depth is 1 for me, 2 for children, 3 for ...

    if (scope.depth > 0 and path_length >= scope.depth) {
        // debug("not in scope: path length: {} scope:{}", path_length, scope);
        return false;
    }

    // The scope must name a node-local PC 
    auto& lpcs = node->value.local_pcs();

    // debug("in scope find pcname {}", scope.pcname);
    auto pcit = lpcs.find(scope.pcname);
    if (pcit == lpcs.end()) {
        // debug("not in scope: node has no named lpc in scope:{}", scope);
        // for (auto lit : lpcs) {
        //     debug("\tname: {}", lit.first);
        // }
        return false;
    }

    // The node-local PC must have all the coords
    const auto& ds = pcit->second;
    for (const auto& name : scope.coords) {
        if (!ds.has(name)) {
            // debug("not in scope: lacks coord {} scope:{}", name, scope);
            return false;
        }
    }

    return true;    
}

bool Tree::Points::on_insert(const Tree::Points::node_path_t& path)
{
    auto* node = path.back();

    for (auto [scope,djds] : m_djds) {
        if (! in_scope(scope, node, path.size())) {
            continue;
        }
        
        Dataset& ds = node->value.m_lpcs[scope.pcname];

        djds.append(ds);

        auto kdit = m_nfkds.find(scope);
        if (kdit == m_nfkds.end()) {
            continue;
        }
        kdit->second->append(ds);
    }

    return true;
}

bool Tree::Points::on_remove(const Tree::Points::node_path_t& path)
{
    // brutal remove

    auto* leaf = path.front();
    size_t psize = path.size();

    // debug("Tree::Points::on_remove node path length={}", psize);
    // for (auto const& [name,_] : leaf->value.local_pcs()) {
    //     debug("\tpcname: {}", name);
    // }

    std::vector<Tree::Scope> dead;
    for (auto const& [scope,_] : m_djds) {
        // debug("Tree::Points::on_remove consider scope={}", scope);
        if (! in_scope(scope, leaf, psize)) {
            continue;
        }
        dead.push_back(scope);
    }
    if (dead.empty()) {
        // debug("Tree::Points::on_remove no scope to remove");
        return true;
    }
    
    for (auto scope : dead) {
        m_djds.erase(scope);
        m_nfkds.erase(scope);
    }

    return true;
}
    

const DisjointDataset& Tree::Points::scoped_pc(const Tree::Scope& scope) const
{
    auto it = m_djds.find(scope);
    if (it != m_djds.end()) {
        return it->second;
    }

    // construct and store
    DisjointDataset& djds = m_djds[scope];
    for (auto& nv : m_node->depth(scope.depth)) {
        // local pc dataset
        auto it = nv.m_lpcs.find(scope.pcname);
        if (it == nv.m_lpcs.end()) {
            continue;
        }
        Dataset& ds = it->second;
        // check that it has the coordinate arrays
        std::vector<std::string> have = ds.keys(), want(scope.coords), both;
        std::sort(want.begin(), want.end());
        std::set_intersection(have.begin(), have.end(), want.begin(), want.end(),
                              std::back_inserter(both));

        if (both.size() == want.size()) {
            djds.append(ds);
            continue;
        }
        raise<IndexError>("Tree::Points::scoped_pc %s lacks required coordinate arrays", scope.pcname);
        
    };
    return djds;
}

