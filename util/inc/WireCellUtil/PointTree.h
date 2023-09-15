/**
   A point cloud with points organized in an n-ary tree structure.

   See the Point class.
 */

#ifndef WIRECELLUTIL_POINTTREE
#define WIRECELLUTIL_POINTTREE

#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/PointNFKD.h"
#include "WireCellUtil/NaryTree.h"
#include "WireCellUtil/KDTree.h"


namespace WireCell::PointCloud::Tree {

    /** A point cloud scope describes a descent to collect node-local
     * point cloud data sets which have (at least) the arrays with
     * names provided by the coords.
     */
    struct Scope {

        // The name of the node-local point clouds
        std::string pcname;

        // The list of PC attribute array names to interpret as coordinates.
        name_list_t coords;

        // The depth of the descent.
        size_t depth;

        std::size_t hash() const;
        bool operator==(const Scope& other) const;
        bool operator!=(const Scope& other) const;
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


    /** Points is a payload value type for a NaryTree::Node<Points> node.

        A Points stores node-local point clouds by name, provides
        results of a scoped query on the n-ary tree of node-local
        point clouds in the form of "disjoint dataset" and allows
        forming k-d trees upon these.

        See Scope, DisjointDataset and NFKD for more explanation.

     */
    class Points : public NaryTree::Notified<Points> {
        
        // This is a helper to interface with nanoflann.  It has
        // typeless and typefull inheritance layers.  It gets a const
        // reference to a DisjointDataset that we construct via a
        // scoped descent.
        using kdtree_base_t = NFKD;
        using kdtree_base_ptr = std::unique_ptr<kdtree_base_t>;

      public:

        using pointcloud_t = Dataset;
        using named_pointclouds_t = std::map<std::string, pointcloud_t>;

        using node_t = NaryTree::Node<Points>;
        using node_path_t = std::vector<node_t*>;
        template<typename ElementType,
                 typename DistanceTraits,
                 typename IndexTraits>
        using kdtree_t = NFKDT<ElementType, DistanceTraits, IndexTraits>;


        Points() = default;
        virtual ~Points() = default;

        // Copy constructor disabled due to holding unique k-d tree 
        Points(const Points& other) = delete;
        // move is okay
        Points(Points&& other) = default;

        Points& operator=(const Points& other) = delete;
        Points& operator=(Points&& other) = default;

        // Construct with local point clouds by copy
        explicit Points(const named_pointclouds_t& pcs)
            : m_lpcs(pcs.begin(), pcs.end()) {}

        // Construct with local point clouds by move
        explicit Points(named_pointclouds_t&& pcs)
            : m_lpcs(std::move(pcs)) {}

        const node_t* node() const { return m_node; };
        node_t* node() { return m_node; };

        // Access the set of point clouds local to this node.
        const named_pointclouds_t& local_pcs() const { return m_lpcs; }

        /// Access a scoped PC.
        const DisjointDataset& scoped_pc(const Scope& scope) const;

        // Access the scoped k-d tree.
        template<typename ElementType=double,
                 typename DistanceTraits=KDTree::DistanceL2Simple,
                 typename IndexTraits=KDTree::IndexDynamic>
        const kdtree_t<ElementType, DistanceTraits, IndexTraits>&
        scoped_kd(const Scope& scope) const {
            using kd_t = kdtree_t<ElementType, DistanceTraits, IndexTraits>;
            auto it = m_nfkds.find(scope);
            if (it != m_nfkds.end()) {
                const auto* ptr = it->second.get();
                const auto* dptr = dynamic_cast<const kd_t*>(ptr);
                if (!dptr) {
                    raise<ValueError>("Tree::Points::scoped_kd(): type collision");
                }
                return *dptr;
            }
            const DisjointDataset& dds = scoped_pc(scope);
            kd_t* ptr = new kd_t(dds, scope.coords);
            m_nfkds[scope] = kdtree_base_ptr(ptr);
            return *ptr;

        }

        // Receive notification from n-ary tree to learn of our node.
        virtual void on_construct(node_t* node);
        // Receive notification from n-ary tree to update existing
        // NFKDs if node is in any existing scope.
        virtual bool on_insert(const node_path_t& path);
        // FIXME: remove is not well supported and currently will
        // simply invalidate any in-scope NFKDs.
        virtual bool on_remove(const node_path_t& path);

      private:

        // our node
        node_t* m_node {nullptr};
        // our node-local point clouds
        named_pointclouds_t m_lpcs;

        // nanoflann k-d tree interfaces for a given scope.
        mutable std::unordered_map<Scope, DisjointDataset> m_dds;
        mutable std::unordered_map<Scope, kdtree_base_ptr> m_nfkds;
    };

}

#endif
