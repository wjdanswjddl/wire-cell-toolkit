/**
   A point cloud with points organized in an n-ary tree structure.

   See the Point class.
 */

#ifndef WIRECELLUTIL_POINTTREE
#define WIRECELLUTIL_POINTTREE

#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/PointCloudCoordinates.h"
#include "WireCellUtil/PointCloudDisjoint.h"
#include "WireCellUtil/NFKD.h"
#include "WireCellUtil/NaryTree.h"
#include "WireCellUtil/KDTree.h"
#include <ostream>

namespace WireCell::PointCloud::Tree {

    /** A point cloud scope describes a descent to collect node-local
     * point cloud data sets which have (at least) the arrays with
     * names provided by the coords.
     */
    struct Scope {

        // The name of the node-local point clouds
        std::string pcname{""};

        // The list of PC attribute array names to interpret as coordinates.
        name_list_t coords{};

        // The depth of the descent.
        size_t depth{0};

        std::size_t hash() const;
        bool operator==(const Scope& other) const;
        bool operator!=(const Scope& other) const;

    };
    std::ostream& operator<<(std::ostream& o, Scope const& s);


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

    struct KDTreeBase {

        // Append more points.
        virtual void append(const Dataset& ds) = 0;
    };


    // Bind together a disjoint store of selection coordinates and
    // their k-d tree. 
    template<typename ValueType>
    struct KDTree : public KDTreeBase {

        using value_type = ValueType;
        using point_type = std::vector<ValueType>;
        using point_group = coordinates<point_type>;
        using group_vector = std::vector<point_group>;
        
        using kdtree_type = NFKD::Tree<typename point_group::iterator>;

        group_vector store;
        kdtree_type kdtree;
        name_list_t names;

        explicit KDTree(const name_list_t& names)
            : kdtree(names.size())
            , names(names)
        {
        }

        KDTree(const DisjointDataset& djds, const name_list_t& names)
            : kdtree(names.size())
            , names(names)
        {
            for (const Dataset& ds : djds.values()) {
                append(ds);
            }
        }

        
        virtual void append(const Dataset& ds) {
            point_group grp(ds.selection(names));
            store.push_back(grp);
            kdtree.append(grp.begin(), grp.end());
        }
    };


    /** Points is a payload value type for a NaryTree::Node<Points> node.

        A Points stores node-local point clouds by name, provides
        results of a scoped query on the n-ary tree of node-local
        point clouds in the form of "disjoint dataset" and allows
        forming k-d trees upon these.

        See Scope, DisjointDataset and NFKD for more explanation.

     */
    class Points : public NaryTree::Notified<Points> {
        
      public:


        using node_t = NaryTree::Node<Points>;
        using node_ptr = std::unique_ptr<node_t>;
        using node_path_t = std::vector<node_t*>;

        using pointcloud_t = Dataset;
        using named_pointclouds_t = std::map<std::string, pointcloud_t>;

        template<typename ValueType>
        using kdtree_t = typename KDTree<ValueType>::kdtree_type;

        Points() = default;
        virtual ~Points() = default;

        // Copy constructor disabled due to holding unique k-d tree 
        Points(const Points& other) = delete;
        /// Move constructor.
        Points(Points&& other) = default;

        /// Copy assignment is deleted.
        Points& operator=(const Points& other) = delete;
        /// Move assignment
        Points& operator=(Points&& other) = default;

        /// Construct with local point clouds by copy
        explicit Points(const named_pointclouds_t& pcs)
            : m_lpcs(pcs.begin(), pcs.end()) {}

        /// Construct with local point clouds by move
        explicit Points(named_pointclouds_t&& pcs)
            : m_lpcs(std::move(pcs)) {}

        /// Access the node that holds us, if any.
        const node_t* node() const { return m_node; };
        node_t* node() { return m_node; };

        /// Access the set of point clouds local to this node.
        const named_pointclouds_t& local_pcs() const { return m_lpcs; }

        /// Access local PCs in mutable form.  Note, manipulating the
        /// PCs directly may circumvent their use in up-tree scoped
        /// PCs.
        // named_pointclouds_t& local_pcs() { return m_lpcs; }

        /// Access a scoped PC.
        const DisjointDataset& scoped_pc(const Scope& scope) const;
        
        /// Access the scoped k-d tree.
        template<typename ValueType=double>
        const kdtree_t<ValueType>&
        scoped_kd(const Scope& scope) const {
            using kd_t = KDTree<ValueType>;
            auto it = m_nfkds.find(scope);
            if (it != m_nfkds.end()) {
                const auto* ptr = it->second.get();
                const auto* dptr = dynamic_cast<const kd_t*>(ptr);
                if (!dptr) {
                    raise<ValueError>("Tree::Points::scoped_kd(): type collision");
                }
                return dptr->kdtree;
            }
            const DisjointDataset& dds = scoped_pc(scope);
            kd_t* ptr = new kd_t(dds, scope.coords);
            m_nfkds[scope] = std::unique_ptr<KDTreeBase>(ptr);
            return ptr->kdtree;

        }

        // Receive notification from n-ary tree to learn of our node.
        virtual void on_construct(node_t* node);

        // Receive notification from n-ary tree to update existing
        // NFKDs if node is in any existing scope.
        virtual bool on_insert(const node_path_t& path);

        // This is a brutal response to a removed node.  Any scope
        // containing the removed node will be removed from the cached
        // scoped data sets and k-d trees.  This will invalidate any
        // references and iterators from these objects that the caller
        // may be holding.
        virtual bool on_remove(const node_path_t& path);

      private:

        // our node
        node_t* m_node {nullptr};
        // our node-local point clouds
        named_pointclouds_t m_lpcs;

        // nanoflann k-d tree interfaces for a given scope.
        mutable std::unordered_map<Scope, DisjointDataset> m_djds;
        mutable std::unordered_map<Scope, std::unique_ptr<KDTreeBase>> m_nfkds;
    };

}

#endif
