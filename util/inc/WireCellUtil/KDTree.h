/** k-d tree operations for fast neighborhood finding on point clouds

 */

#ifndef WIRECELL_UTIL_KDTREE
#define WIRECELL_UTIL_KDTREE

#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/nanoflann.hpp"
#include <boost/algorithm/string/join.hpp>

namespace WireCell::KDTree {

    /** Types of distance metric, first good for normal concept of
        cartesian L2 distance on small dimensions.  Note, each metric
        is in its own units, eg L2 is units of [length^2].  It's up to
        the user setting the distance metric to put it into expected
        units.
    */
    enum class Metric { l2simple, l1, l2, so2, so3 };

    /// Provide nanoflan index adaptor traits to make class templates
    /// easier to supply.
    struct IndexTraits {    };
    struct IndexStatic : public IndexTraits {
        template <typename Distance, 
                  class DatasetAdaptor, int32_t DIM = -1,
                  typename IndexType = size_t>
        struct traits {
            using index_t = nanoflann::KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, IndexType>;
        };
    };
    struct IndexDynamic : public IndexTraits {
        template <typename Distance, 
                  class DatasetAdaptor, int32_t DIM = -1,
                  typename IndexType = size_t>
        struct traits {
            using index_t = nanoflann::KDTreeSingleIndexDynamicAdaptor<Distance, DatasetAdaptor, DIM, IndexType>;
        };
    };
    /// Likewise for distance.  Here, nanoflann provides them so we
    /// simply forward their names for the ones supported here.
    using DistanceTraits = nanoflann::Metric;
    using DistanceL1 = nanoflann::metric_L1;
    using DistanceL2 = nanoflann::metric_L2;
    using DistanceL2Simple = nanoflann::metric_L2_Simple;    

    /// Query result type
    template<typename ElementType>
    struct Results {
        Results() = default;
        explicit Results(size_t capacity)
            : index(capacity) , distance(capacity) 
        { } 

        /**
           The indices the found points that count into the
           original Dataset arrays which providing the point
           cloud coordinates
        */
        std::vector<size_t> index;
        /**
           The distance from query point to the found point at
           corresponding index.
        */
        std::vector<ElementType> distance;
    };

    struct QueryBase {

        virtual ~QueryBase() {};
        virtual bool dynamic() const = 0;
        virtual Metric metric() const = 0;

    };


    /** Query a dataset via k-d tree.

        This is based on but does not expose nanoflann.  It restricts
        to a smaller "type space" than supported by nanoflann by
        assuming coordinate and distance are both represented by the
        same type (ElementType) and indexing uses type size_t.  It
        also hides from the user the details of nanoflann adaptors
        types: dataset, distance and index types.  Instead the user
        may set these dynamically.

        The k-d tree query object will be connected to the Dataset so
        that when Dataet::append() is called the k-d tree will be
        updated.
     */
    template<typename ElementType>
    class Query : public QueryBase {
      public:

        virtual ~Query() {}

        using element_t = ElementType;
        using point_t = std::vector<ElementType>;
        using distance_t = ElementType;
        using results_t = Results<ElementType>;

        /** Return the k-nearest neighbor points from the query point.

            Points are returned in the Result as a vector of indices
            into the original dataset.  Result also includes distances
            in an array matched one-to-one with indices.
        */
        virtual results_t knn(size_t k, const point_t& query_point) = 0;

        /** Return the neighbor points within a metric distance d from query point.

            Note: the units in which r is expressed must match the
            metric.  If using an L2 metric, r is in units of
            [length^2].

            See knn() for return value and arguments
        */
        virtual results_t radius(distance_t d, const point_t& query_point)= 0;

        /** Update the k-d tree, if it is dynamic, for points that
         * have been added to the underlying point cloud with their
         * indices in the closed/inclusive range [beg,end]. */
        virtual void update(size_t beg, size_t end) {};
    };


    template<typename ElementType>
    using query_ptr_t = std::unique_ptr<Query<ElementType>>;

    // Return a k-d tree query object on a dataset.  If dynamic is
    // true, the k-d tree is updated automatically when points are
    // appended to the dataset.
    template<typename ElementType>
    query_ptr_t<ElementType>
    query(PointCloud::Dataset& ds,
          const PointCloud::Dataset::name_list_t& names,
          bool dynamic = false,
          Metric mtype = Metric::l2simple);
    template<>
    query_ptr_t<int>
    query<int>(PointCloud::Dataset& ds,
               const PointCloud::Dataset::name_list_t& names,
               bool dynamic, Metric mtype);
    template<>
    query_ptr_t<float>
    query<float>(PointCloud::Dataset& ds,
                 const PointCloud::Dataset::name_list_t& names,
                 bool dynamic, Metric mtype);
    template<>
    query_ptr_t<double>
    query<double>(PointCloud::Dataset& ds,
                  const PointCloud::Dataset::name_list_t& names,
                  bool dynamic, Metric mtype);

    /** Cache multiple k-d tree queries on a given dataset the may
     * differ in their coordinate attributes, metric types and whether
     * the k-d tree should be static or dynamic.
     */
    class MultiQuery {
        PointCloud::Dataset* m_dataset;
        std::map<std::string, std::shared_ptr<QueryBase>> m_indices;

      public:

        template<typename ElementType>
        using query_ptr_t = std::shared_ptr<Query<ElementType>>;

        MultiQuery(PointCloud::Dataset& dataset)
            : m_dataset(&dataset)
        {}

        MultiQuery(const MultiQuery& rhs)
            : m_dataset(rhs.m_dataset)
        {}

        MultiQuery& operator=(const MultiQuery& rhs)
        {
            m_dataset = rhs.m_dataset;
            return *this;
        }

        const PointCloud::Dataset& dataset() const
        {
            return *m_dataset;
        }

        PointCloud::Dataset& dataset()
        {
            return *m_dataset;
        }

        std::string make_key(const PointCloud::Dataset::name_list_t& names,
                             bool dynamic, Metric mtype)
        {
            PointCloud::Dataset::name_list_t keys(names.begin(),
                                                  names.end());
            keys.push_back(std::to_string((int) dynamic));
            keys.push_back(std::to_string(static_cast<std::underlying_type_t<Metric>>(mtype)));
            return boost::algorithm::join(keys, ",");
        }

        // Return a new or existing query matching parameters.
        template<typename ElementType>
        query_ptr_t<ElementType>
        get(const PointCloud::Dataset::name_list_t& names, bool dynamic=false, Metric mtype = Metric::l2simple)
        {
            auto key = make_key(names, dynamic, mtype);
            auto it = m_indices.find(key);

            if (it == m_indices.end()) { // create
                auto unique = query<ElementType>(*m_dataset, names,
                                                 dynamic, mtype);
                query_ptr_t<ElementType> ret = std::move(unique);
                m_indices[key] = ret;
                return ret;
            }
            
            std::shared_ptr<QueryBase> base = it->second;
            return std::dynamic_pointer_cast<Query<ElementType>>(base);
        }

    };

} // WireCell::KDTree

#endif

