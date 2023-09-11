#ifndef WIRECELL_POINTTREE
#define WIRECELL_POINTTREE

#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/KDTree.h"
#include "WireCellUtil/NaryTree.h"

#include <vector>
#include <memory>
#include <string>

namespace WireCell::PointCloud {

        
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
        explicit NodeValue(const pointcloud_set& npcs) : npcs(npcs) {}

        // Move value
        explicit NodeValue(pointcloud_set&& npcs) : npcs(std::move(npcs)) {}

        /** Access the named point cloud set.
         */
        const pointcloud_set& pcs() const { return npcs; } 

        /** A "scope" describes a subset of the point cloud tree.

            The scope spans a depth-first descent of the tree and is
            delineated by three values:

            - name :: only local point clouds of the given name are
              in the scope.

            - attrs :: the point clouds, if they exist, must contain
              these attributes and only these attributes are in scope.

            - depth :: the number of descent levels that are in scope.
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

            size_t depth{0};

            std::size_t hash() const;
        };


        /** A point cloud and its k-d tree multi-query

            When associated with a scope, the set of attributes of the
            pc will be limited to those given by the scope's "attrs".

         */
        struct Payload {
            pointcloud_t pc;
            kdtree_cache_t kc;
            explicit Payload(const pointcloud_t& pc_) : pc(pc_), kc(pc) {};
            explicit Payload(pointcloud_t&& pc_) : pc(std::move(pc_)), kc(pc) {};
        };

        /** Return a scoped payload.
         */
        const Payload& payload(const Scope& scope) const;



        virtual void on_construct(node_type* node);
        virtual bool on_insert(const std::vector<node_type*>& path);
        virtual bool on_remove(const std::vector<node_type*>& path);

      private:
        pointcloud_set npcs;


        // Cache scopes
        mutable std::unordered_map<Scope, Payload> cache;

    };


}
namespace std {
    template<>
    struct hash<WireCell::PointCloud::NodeValue::Scope> {
        std::size_t operator()(const WireCell::PointCloud::NodeValue::Scope& scope) const {
            return scope.hash();
        }
    };
}
#endif
