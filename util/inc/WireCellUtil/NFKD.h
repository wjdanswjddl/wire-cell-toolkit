// An iterator based interface to nanoflan.

#ifndef WIRECELLUTIL_NFKD
#define WIRECELLUTIL_NFKD

#include "WireCellUtil/nanoflann.hpp"
#include "WireCellUtil/DetectionIdiom.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/DisjointRange.h"

#include "WireCellUtil/Logging.h" // debug

#include <map>

namespace WireCell::NFKD {

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
    // L1 - sum of absolute linear difference on each coordinate.  The
    // radius and distances in results sets are in units of [LENGTH].
    using DistanceL1 = nanoflann::metric_L1;
    // L2 - sum of squared difference on each coordinate.  NOTE: the
    // value for this metric is in length units SQUARED.  The "radius"
    // given to a radius() query and the distances between the query
    // point and the points in the result set (including that from
    // knn() query) are all in units of [LENGTH]^2.  They are not NOT
    // in units of [LENGTH].
    using DistanceL2 = nanoflann::metric_L2;
    // L2 but optimize for low-dimension L2, 2D or 3D.  This is
    // default.
    using DistanceL2Simple = nanoflann::metric_L2_Simple;    


    // Interface to a k-d tree with points accessed by an iterator
    // range.
    template<typename PointRange
             ,typename IndexTraits = IndexDynamic
             ,typename DistanceTraits = DistanceL2Simple
             >    
    class Tree {

      public:

        // We track appended point ranges in a disjoint_range.
        using points_t = disjoint_range<PointRange>;

        // k-d tree queries will be in terms of iterators into that
        // disjoint range.
        using iterator = typename points_t::iterator;
        using const_iterator = typename points_t::const_iterator;

        // The iterator value type is what we accept as a point for
        // k-d tree queries.
        using point_type = typename iterator::value_type;

        // The scalar numeric type for the coordinates of a point.
        using element_type = typename point_type::value_type;

        using self_type = Tree<PointRange, IndexTraits, DistanceTraits>;

        // nanoflann types
        using metric_type = typename DistanceTraits::template traits<element_type, self_type>::distance_t;
        using index_type = typename IndexTraits::template traits<metric_type, self_type>::index_t;
        using distance_type = typename metric_type::DistanceType;

        /// A result of a k-d query.  The first part of a pair gives
        /// the iterator to a point in the point cloud.  The second
        /// gives the distance from the query point to that point.
        using result_item = std::pair<iterator, distance_type>;
        using results_type = std::vector<result_item>;
        using const_result_item = std::pair<const_iterator, distance_type>;
        using const_results_type = std::vector<const_result_item>;

        explicit Tree(size_t dimensionality)
            : m_index(dimensionality, *this)
        {
        }

        // Create with one initial range
        template<typename RangeIterator>
        Tree(size_t dimensionality, RangeIterator beg, RangeIterator end)
            : m_points(beg, end)
            , m_index(dimensionality, *this)
        {
        }

        // Create with collection of ranges
        template<typename Container>
        Tree(size_t dimensionality, Container& con)
            : m_points(con)
            , m_index(dimensionality, *this)
        {
        }

        ~Tree()
        {
        }

        points_t& points() { return m_points; }
        const points_t& points() const { return m_points; }

        size_t major_index(iterator it) const {
            return m_points.major_index(it);
        }
        size_t major_index(const_iterator it) const {
            return m_points.major_index(it);
        }
        size_t minor_index(iterator it) const {
            return m_points.minor_index(it);
        }
        size_t minor_index(const_iterator it) const {
            return m_points.minor_index(it);
        }

        // Return the number calls made so far to resolve a point
        // coordinate.  Mostly for debugging/perfing.
        size_t point_calls() const {
            return m_point_calls;
        }
        
        // Append a range of points - can only be called if IndexTraits = IndexDynamic.
        template<typename RangeIterator>
        void append(RangeIterator beg, RangeIterator end)
        {
            const size_t oldsize = m_points.size();
            const size_t adding = std::distance(beg, end);
            m_points.append(beg, end);
            this->addn<index_type>(oldsize, adding);
        }
        template<typename Range>
        void append(Range r)
        {
            append(r.begin(), r.end());
        }

        template<typename VectorLike>
        results_type knn(size_t kay, const VectorLike& query_point) {
            results_type ret;
            if (!kay) { return ret; }

            std::vector<size_t> indices(kay,0);
            std::vector<distance_type> distances(kay, 0);
            nanoflann::KNNResultSet<element_type> nf(kay);
            nf.init(&indices[0], &distances[0]);
            m_index.findNeighbors(nf, query_point.data(),
                                  nanoflann::SearchParameters());
            const size_t nfound = nf.size();
            for (size_t ind=0; ind<nfound; ++ind) {
                auto it = m_points.begin();
                it += indices[ind];
                ret.push_back(std::make_pair(it, distances[ind]));
            }
            return ret;
        }
        template<typename VectorLike>
        const_results_type knn(size_t kay, const VectorLike& query_point) const {
            const_results_type ret;
            if (!kay) { return ret; }

            std::vector<size_t> indices(kay,0);
            std::vector<distance_type> distances(kay, 0);
            nanoflann::KNNResultSet<element_type> nf(kay);
            nf.init(&indices[0], &distances[0]);
            m_index.findNeighbors(nf, query_point.data(),
                                  nanoflann::SearchParameters());
            const size_t nfound = nf.size();
            for (size_t ind=0; ind<nfound; ++ind) {
                auto it = m_points.begin();
                it += indices[ind];
                ret.push_back(std::make_pair(it, distances[ind]));
            }
            return ret;
        }

        template<typename VectorLike>
        results_type radius(distance_type rad, const VectorLike& query_point) {
            std::vector<nanoflann::ResultItem<size_t, element_type>> res;
            nanoflann::RadiusResultSet<element_type, size_t> rs(rad, res);
            m_index.findNeighbors(rs, query_point.data());

            results_type ret;
            for (const auto& [index, distance] : res) {
                auto it = m_points.begin();
                it += index;
                ret.push_back(std::make_pair(it, distance));
            }
            return ret;
        }
        template<typename VectorLike>
        const_results_type radius(distance_type rad, const VectorLike& query_point) const {
            std::vector<nanoflann::ResultItem<size_t, element_type>> res;
            nanoflann::RadiusResultSet<element_type, size_t> rs(rad, res);
            m_index.findNeighbors(rs, query_point.data());

            const_results_type ret;
            for (const auto& [index, distance] : res) {
                auto it = m_points.begin();
                it += index;
                ret.push_back(std::make_pair(it, distance));
            }
            return ret;
        }

        // nanoflann dataset adaptor API.
        inline size_t kdtree_get_point_count() const {
            // spdlog::debug("NFKD: {} kdtree_get_point_count size={}", (void*)this, m_points.size());
            return m_points.size();
        }
        template <class BBOX>
        bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

        // unwantedly, nanoflan deals in indices
        inline element_type kdtree_get_pt(size_t idx, size_t dim) const {
            // spdlog::debug("NFKD: {} getting pt({}/{},{})", (void*)this, idx,m_points.size(),dim);
            const auto& pt = m_points.at(idx);
            const element_type val = pt.at(dim);
            ++m_point_calls;
            // spdlog::debug("NFKD: get pt({}/{},{})={}", idx,m_points.size(),dim,val);
            return val;
        }

      private:
        points_t m_points;

        index_type m_index;
        mutable size_t m_point_calls{0};

        // discovery idiom to figure out how to add points given the
        // k-d tree is dynamic or static.
        template <typename T, typename ...Ts>
        using addpoints_type = decltype(std::declval<T>().addPoints(std::declval<Ts>()...));

        template<typename T>
        using has_addpoints = is_detected<addpoints_type, T, uint32_t, uint32_t>;

        // Dynamic index case
        template <class T, std::enable_if_t<has_addpoints<T>::value>* = nullptr>
        void addn(size_t beg, size_t n) {
            if (n) {
                // spdlog::debug("NFKD: {} addn({},+{})", (void*)this, beg, n);
                this->m_index.addPoints(beg, beg+n-1);
            }
        }

        // Static index case
        template <class T, std::enable_if_t<!has_addpoints<T>::value>* = nullptr>
        void addn(size_t beg, size_t n) {
            raise<LogicError>("NFKD::Tree: static index can not have points added");
            return; // no-op
            // Note, we could instead clear and rebuild the k-d tree
        }

    };
     

}

#endif
