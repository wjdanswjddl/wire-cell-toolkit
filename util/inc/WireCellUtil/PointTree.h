/**
   A point cloud with points organized in an n-ary tree structure.

   See the Point class.
 */

#ifndef WIRECELLUTIL_POINTTREE
#define WIRECELLUTIL_POINTTREE

#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/PointCloudCoordinates.h"
#include "WireCellUtil/NFKD.h"
#include "WireCellUtil/NaryTree.h"
#include "WireCellUtil/KDTree.h"

#include <boost/range/adaptors.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <ostream>

namespace WireCell::PointCloud::Tree {

    /** A point cloud scope describes selection of point cloud data
     * formed from a subset of tree nodes reached by a depth-first
     * descent.  
     */
    struct Scope {

        // The name of the node-local point clouds.
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

    // An atomic, contiguous point cloud.
    using pointcloud_t = Dataset;

    // A set of node-local named point clouds.
    using named_pointclouds_t = std::map<std::string, pointcloud_t>;

    // A scoped point cloud is an ordered collection of references to
    // datasets held by node-local named point clouds in a given
    // scope.
    using pointcloud_ref = std::reference_wrapper<pointcloud_t>;
    using scoped_pointcloud_t = std::vector<pointcloud_ref>;
    
    // Type-free base class independent on the numeric coordinate
    // element type.
    struct KDTreeBase {
        virtual ~KDTreeBase() {}

        // Append more points.
        virtual void append(pointcloud_t& pc) = 0;

    };


    // Bind together a disjoint store of selection coordinates and
    // their k-d tree. 
    template<typename ElementType>
    struct KDTree : public KDTreeBase {

        using element_type = ElementType;
        using point_type = coordinate_point<ElementType>;
        using point_group = coordinate_range<point_type>;
        
        using nfkdtree_type = NFKD::Tree<point_group>;

        std::vector<selection_t> store;
        nfkdtree_type nfkdtree;

        name_list_t names;

        explicit KDTree(const name_list_t& names)
            : nfkdtree(names.size())
            , names(names)
        {
        }
        virtual ~KDTree() {}

        // Prime with a collection or range of datasets
        KDTree(scoped_pointcloud_t& pcs, const name_list_t& names)
            : nfkdtree(names.size())
            , names(names)
        {
            for (pointcloud_t& pc : pcs) {
                append(pc);
            }
        }
        
        virtual void append(pointcloud_t& pc) {
            selection_t sel = pc.selection(names);
            store.push_back(sel); // keep alive
            nfkdtree.append(point_group(store.back()));
        }

    };

    /** Points is a payload value type for a NaryTree::Node.

        A Points stores a set of point clouds local to the node.
        Individual point clouds in the set are accessed by a name of
        type string.

        A Points also provides access to "scoped" objects.  A scoped
        object is formed as a concatenation of objects encountered
        during a depth-first descent that is goverend by the "scope".

        Scoped objects include

        - scoped point cloud :: a "disjoint dataset" that represents a
          flattened concatenation of all node-local point clouds in
          the given scope.

        - scoped k-d tree :: a k-d tree formed on a scoped point
          cloud.

        See Scope and NFKD for more explanation.

     */
    class Points : public NaryTree::Notified<Points> {
        
      public:

        using node_t = NaryTree::Node<Points>;
        using node_ptr = std::unique_ptr<node_t>;
        using node_path_t = std::vector<node_t*>;

        // template<typename ElementType>
        // using kdtree_t = typename KDTree<ElementType>::kdtree_type;

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
        scoped_pointcloud_t& scoped_pc(const Scope& scope);
        
        /// Access the scoped k-d tree.
        template<typename ElementType>
        using nfkd_t = typename KDTree<ElementType>::nfkdtree_type;

        template<typename ElementType=double>
        nfkd_t<ElementType> &
        scoped_kd(const Scope& scope) {
            using kd_t = KDTree<ElementType>;
            auto it = m_scoped_kds.find(scope);
            if (it != m_scoped_kds.end()) {
                auto* ptr = it->second.get();
                auto* dptr = dynamic_cast<kd_t*>(ptr);
                if (!dptr) {
                    raise<ValueError>("Tree::Points::scoped_kd(): type collision");
                }
                return dptr->nfkdtree;
            }
            auto pcr = scoped_pc(scope);
            kd_t* ptr = new kd_t(pcr, scope.coords);
            m_scoped_kds[scope] = std::unique_ptr<KDTreeBase>(ptr);
            return ptr->nfkdtree;

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
        mutable std::unordered_map<Scope, scoped_pointcloud_t> m_scoped_pcs;
        mutable std::unordered_map<Scope, std::unique_ptr<KDTreeBase>> m_scoped_kds;
    };

}

#endif
