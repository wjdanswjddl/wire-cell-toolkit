/** k-d tree operations for fast neighborhood finding on point clouds

 */

#ifndef WIRECELL_UTIL_KDTREE
#define WIRECELL_UTIL_KDTREE

#include "WireCellUtil/PointCloud.h"

namespace WireCell::KDTree {

    /** Types of distance metric, first good for normal concept of
        cartesian L2 distance on small dimensions.  Note, each metric
        is in its own units, eg L2 is units of [length^2].  It's up to
        the user setting the distance metric to put it into expected
        units.
    */
    enum class Metric { l2simple, l1, l2, so2, so3 };

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
    class Query {
      public:

        virtual ~Query() {}

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

    };

    std::unique_ptr<Query<int>>
    query_int(PointCloud::Dataset& dataset,
              const PointCloud::name_list_t& selection,
              bool dynamic = false,
              Metric mtype = Metric::l2simple);

    std::unique_ptr<Query<float>>
    query_float(PointCloud::Dataset& dataset,
                const PointCloud::name_list_t& selection,
                bool dynamic = false,
                Metric mtype = Metric::l2simple);

    std::unique_ptr<Query<double>>
    query_double(PointCloud::Dataset& dataset,
                 const PointCloud::name_list_t& selection,
                 bool dynamic = false,
                 Metric mtype = Metric::l2simple);


} // WireCell::KDTree

#endif

