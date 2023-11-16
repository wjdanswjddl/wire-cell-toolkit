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

    // The node-local PC must have all the coords.  coords may be
    // empty such as when the tested scope is a key in cached point
    // clouds (m_scoped_pcs).
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

    for (auto& [scope,pcstore] : m_scoped_pcs) {
        if (! in_scope(scope, node, path.size())) {
            continue;
        }
        auto it = node->value.m_lpcs.find(scope.pcname);
        if (it == node->value.m_lpcs.end()) {
            raise<WireCell::LogicError>("node in scope but no local PC of name " + scope.pcname);
        }
        Dataset& pc = it->second;

        // add to scoped PCs
        pcstore.push_back(std::ref(pc));

        // append to scopped k-d tree if exists
        auto kdit = m_scoped_kds.find(scope);
        if (kdit == m_scoped_kds.end()) {
            continue;
        }

        kdit->second->append(pc);
    }

    return true;
}

template<typename Store>
void purge(Store& store, const Tree::Points::node_path_t& path)
{
    auto* leaf = path.front();
    size_t psize = path.size();

    std::vector<Tree::Scope> dead;
    for (auto const& [scope,_] : store) {
        if (in_scope(scope, leaf, psize)) {
            dead.push_back(scope);
        }
    }
    for (auto scope : dead) {
        store.erase(scope);
    }
}

bool Tree::Points::on_remove(const Tree::Points::node_path_t& path)
{
    purge(m_scoped_kds, path);
    purge(m_scoped_pcs, path);

    return true;                // continue ascent
}

static void assure_arrays(const std::vector<std::string>& have, // ds keys
                          const Tree::Scope& scope)
{
    // check that it has the coordinate arrays
    std::vector<std::string> want(scope.coords), both, missing;
    std::sort(want.begin(), want.end());
    std::set_intersection(have.begin(), have.end(), want.begin(), want.end(),
                          std::back_inserter(both));
    if (both.size() == want.size()) {
        return;                 // arrays exist
    }

    // collect missing for exception message
    std::set_difference(have.begin(), have.end(), want.begin(), want.end(),
                        std::back_inserter(missing));
    std::string s;
    for (const auto& m : missing) {
        s += " " + m;
    }
    WireCell::raise<WireCell::IndexError>("Tree::Points data set missing arrays \"%s\" from scope %s",
                                          s, scope);    
}


WireCell::PointCloud::Tree::scoped_pointcloud_t&
Tree::Points::scoped_pc(const Tree::Scope& scope)
{
    // Since we cache full dataset we do not include coords in the
    // lookup key to encourage sharing.  But we will still assure it
    // holds the scope's arrays.
    Tree::Scope dscope = scope;
    dscope.coords.clear();

    // cache hit?
    auto it = m_scoped_pcs.find(dscope);
    if (it != m_scoped_pcs.end()) {
        // Repeat this check as we may do initial construction with a
        // scope that has a different set of coordinate arrays.
        scoped_pointcloud_t& pcstore = it->second;
        for (Dataset& pc : pcstore) {
            assure_arrays(pc.keys(), scope);            
        }
        return pcstore;
    }

    // construct, assure and cache.
    scoped_pointcloud_t& pcstore = m_scoped_pcs[dscope];
    for (auto& node : m_node->depth(scope.depth)) {
        auto& value = node.value;
        // local pc dataset
        auto it = value.m_lpcs.find(scope.pcname);
        if (it == value.m_lpcs.end()) {
            continue;           // it is okay if node lacks PC
        }

        // Check for coordintate arrays on first construction. 
        Dataset& pc = it->second;
        assure_arrays(pc.keys(), scope);
        pcstore.push_back(std::ref(pc));
    };
    return pcstore;
}

