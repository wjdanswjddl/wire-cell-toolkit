#ifndef WIRECELL_POINTTREE
#define WIRECELL_POINTTREE

#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/KDTree.h"
#include "WireCellUtil/NaryTree.h"

#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace WireCell::PointCloud::Tree {

    /** A "scope" describes a subset of the point cloud tree.

        The scope spans a depth-first descent rooted in a given node.

        The scope is delineated by three values:

        - name :: the name of local point clouds that are selected.

        - attrs :: list of PC attributes.

        If a local PC of a given name is found it must provide these
        attributes.

        - depth :: descent extent.

        The depth values are as described:

        - 0 = unlimited depth, descend entire tree rooted on this node.
        - 1 = the "local" PC is returned, similar to at().
        - 2 = same as 1 plus PCs from children
        - 3 = same as 2 plus PCs grandchildren
        - N = etc.

    */
    struct Scope {
        std::string name;
        name_list_t attrs;
        size_t depth;
        std::size_t hash() const;

        bool operator==(const Scope& other) const {
            if (depth != other.depth) return false;
            if (name != other.name) return false;
            if (attrs.size() != other.attrs.size()) return false;
            for (size_t ind = 0; ind<attrs.size(); ++ind) {
                if (attrs[ind] != other.attrs[ind]) return false;
            }
            return true;
        }
        bool operator!=(const Scope& other) const {
            if (*this == other) return false;
            return true;
        }
    };
}

namespace std {
    template<>
    struct hash<WireCell::PointCloud::Tree::Scope> {
        std::size_t operator()(const WireCell::PointCloud::Tree::Scope& scope) const {
            return scope.hash();
        }
    };
}

namespace WireCell::PointCloud::Tree {

        
    /** An N-ary tree node value holding named point clouds.

    */
    class NodeValue : public NaryTree::Notified<NodeValue> {
      public:

        using pointcloud_t = Dataset;
        // Set of named point clouds
        using pointcloud_set = std::map<std::string, pointcloud_t>;

        // From this one may .get<type>(attrs) to get a k-d tree query
        // object associated with a PC.
        using kdtree_cache_t = KDTree::MultiQuery;

        // The type of node that holds u
        using node_type = NaryTree::Node<NodeValue>;

        // Our node in the tree.  Set in on_construct()
        node_type* node {nullptr};

        NodeValue() = default;
        virtual ~NodeValue() = default;

        // Copy value
        explicit NodeValue(const pointcloud_set& npcs_) : npcs(npcs_.begin(), npcs_.end()) {}

        // Move value
        explicit NodeValue(pointcloud_set&& npcs_) : npcs(std::move(npcs_)) {}

        /** Access the named point cloud set.
         */
        const pointcloud_set& pcs() const { return npcs; } 


        /** A point cloud and its k-d tree multi-query

            When associated with a scope, the set of attributes of the
            pc will be limited to those given by the scope's "attrs".

         */
        struct Payload {
            pointcloud_t pc;
            kdtree_cache_t kdc;
            std::vector<PointCloud::selection_t> lpcs;

            explicit Payload(const pointcloud_t& pc_) : pc(pc_), kdc(pc) {};
            explicit Payload(pointcloud_t&& pc_) : pc(std::move(pc_)), kdc(pc) {};
        };

        /** Return a scoped payload.
         */
        const Payload& payload(const Scope& scope) const;

        /**
           Subclass should provide to receive notification of
           "constructed" action.
         */
        virtual void on_construct(node_type* node);
        /**
           Subclass should provide to receive notification of
           "inserted" action.  Return true to continue to propagate
           toward root node.
         */
        virtual bool on_insert(const std::vector<node_type*>& path);
        /**
           Subclass should provide to receive notification of
           "removing" action.  Return true to continue to propagate
           toward root node.
         */
        virtual bool on_remove(const std::vector<node_type*>& path);

      private:
        pointcloud_set npcs;


        // Cache scopes
        mutable std::unordered_map<Scope, Payload> cache;

    };


}
#endif
