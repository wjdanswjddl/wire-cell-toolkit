// Adaptor to nanoflann over a disjoint dataset

#ifndef WIRECELLUTIL_POINTNFKD
#define WIRECELLUTIL_POINTNFKD

#include "WireCellUtil/PointCloudDisjoint.h"
#include "WireCellUtil/DetectionIdiom.h"
// #include "WireCellUtil/Logging.h"
#include "WireCellUtil/nanoflann.hpp"
#include <unordered_map>

namespace WireCell::PointCloud::Tree {

    /// Base class for interface to nanoflann dataset and index query
    /// adaptors.  This is intended to be constructed via a Points and
    /// not directly by application code.
    class NFKD {
      protected:
        // The thing we glom on to
        const DisjointDataset& m_dds;
      public:
        
        NFKD() = delete;
        NFKD(const NFKD& other) = delete;
        NFKD& operator=(const NFKD& other) = delete;

        explicit NFKD(const DisjointDataset& dds) : m_dds(dds) {}

        // Caller notifies us that the underlying DisjointDataset has
        // had points added.
        virtual void addpoints(size_t beg, size_t end) = 0;

        /// The numeric distance between points for any type of k-d
        /// tree query is chosen to be of type double (even in the
        /// unusual choice that ElementType is integer and the
        /// Manhattan metric is used).
        using distance_t = double;

        /// A result of a k-d query are associated pairs.  The first
        /// part of the pair is gives the index of the point in the
        /// point cloud.  The second gives the distance from the query
        /// point to the point at that index.
        using results_t = std::unordered_map<size_t, distance_t>;

        /// Access the underlying sequence of node-local point cloud
        /// datasets.
        const DisjointDataset& pointclouds() const { return m_dds; }

        // nanoflann dataset adaptor API (one more in KDQueryTyped).
        inline size_t kdtree_get_point_count() const {
            return m_dds.nelements();
        }
        template <class BBOX>
        bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }
    };

    /// Typed interface to nanoflann
    ///
    /// - ElementType provides the numeric type (eg, int, double) of the coordinate arrays.
    ///
    /// - DistanceTraits provides a .traits template giving the type for the distance metric.  Eg nanoflann::metric_L2.
    ///
    /// - IndexTraits provides a .traits template giving the type for the index adaptor.  Eg IndexStatic.
    ///
    template<typename ElementType,
             typename DistanceTraits,
             typename IndexTraits>
    class NFKDT : public NFKD {
      public:

        // The numeric type of coordinate array elements.
        using element_t = ElementType;
        // The coordinate point type.
        using point_t = std::vector<ElementType>;
        // This class is a nanoflann dataset adaptor.
        using dataset_t = NFKDT<ElementType, DistanceTraits, IndexTraits>;
        // Derive the distance metric type.
        using metric_t = typename DistanceTraits::template traits<element_t, dataset_t>::distance_t;
        // Derive the k-d tree index type.
        using index_t = typename IndexTraits::template traits<metric_t, dataset_t>::index_t;

        NFKDT() = delete;
        NFKDT(const NFKDT& other) = delete;
        NFKDT& operator=(const NFKDT& other) = delete;

        /// Construct with a dataset and a list of dataset attribute
        /// array names to interpret as coordinates.  These arrays
        /// must be typed consistent with ElementType.
        explicit NFKDT(const DisjointDataset& dds, const name_list_t& coords)
            : NFKD(dds)
            , m_coords(coords)
            , m_index(coords.size(), *this)
        {
            // fixme: check coords types?
        }

        // FIXME: the underlying k-d tree will become out of sync with
        // the DisjointDataset when if points are added to any DS in
        // the DisjointDataset (appending a new DS is okay).  We may
        // keep sync by setting a callback on each new Dataset in the
        // DisjointDataset to trigger a k-d tree update.  The change
        // in the Dataset may not be an append and thus in general the
        // callback must invalidate the k-d tree.  It can then be
        // remade next time it is requested but it should be cleared
        // in place to that any users holding on to a previously
        // returned reference can see the update.  Alternatively, we
        // may add API to allow the caller to initiate regenerating a
        // k-d tree.

        /** Return the k-nearest neighbor points from the query point.
         */
        virtual results_t knn(size_t kay, const point_t& query_point) const {
            results_t ret(kay);
            if (!kay) { return ret; }

            std::vector<size_t> indices(kay,0);
            std::vector<ElementType> distances(kay, 0);
            nanoflann::KNNResultSet<element_t> nf(kay);
            nf.init(&indices[0], &distances[0]);
            m_index.findNeighbors(nf, query_point.data(),
                                  nanoflann::SearchParameters());
            const size_t nfound = nf.size();
            for (size_t ind=0; ind<nfound; ++ind) {
                ret.insert(std::make_pair(indices[ind], distances[ind]));
            }
            return ret;
        }

        /** Return the neighbor points within a metric distance d from query point.

            IMPORTANT NOTE: the units in which r is expressed must
            match the metric.  If using an L2 metric, r is in units of
            [length^2].
        */
        virtual results_t radius(distance_t rad, const point_t& query_point) const {
            /// nanoflann API change
            // std::vector<std::pair<size_t, element_t>> ids;
            std::vector<nanoflann::ResultItem<size_t, element_t>> res;
            /// The non-underscore dynamic adaptor does not provide radiusSearch().
            // m_index.radiusSearch(query_point.data(), rad, res);

            nanoflann::RadiusResultSet<element_t, size_t> rs(rad, res);
            m_index.findNeighbors(rs, &query_point[0]);

            results_t ret;
            for (const auto& [index, distance] : res) {
                ret.insert(std::make_pair(index, distance));
            }
            return ret;
        }


        // dataset adaptor API
        inline element_t kdtree_get_pt(size_t idx, size_t dim) const
        {
            // FIXME: this is probably a rather expensive lookup....
            //             
            // Can speed things up by maintaining a vector of
            // selection_t or vector of span_t which is updated on
            // each append(ds).
            const auto& dses = m_dds.values();
            // spdlog::debug("kdtree_get_pt({}, {}) dds: [{}] {} {}",
            //               idx, dim, m_dds.npoints(), dses.size(), (void*)&m_dds);
            auto [dind,lind] = m_dds.address(idx); // may throw IndexError
            const Dataset& ds = dses[dind];
            const auto& name = m_coords[dim];
            return ds.get(name).element<element_t>(lind);
        }

        // This must be holder of the DisjointDataset to notify us
        // that it has been enlarged.
        virtual void addpoints(size_t beg, size_t end)
        {
            addpoints_<index_t>(beg,end);
        }

      private:
        name_list_t m_coords;
        index_t m_index;

        // discovery idiom so we only try to call addPoints if our index is dynamic.
        template <typename T, typename ...Ts>
        using addpoints_type = decltype(std::declval<T>().addPoints(std::declval<Ts>()...));

        template<typename T>
        using has_addpoints = is_detected<addpoints_type, T, uint32_t, uint32_t>;

        // Dynamic index case
        template <class T, std::enable_if_t<has_addpoints<T>::value>* = nullptr>
        void addpoints_(size_t beg, size_t end) {
            if (end > beg) {
                this->m_index.addPoints(beg, end-1);
            }
        }

        // Static index case
        template <class T, std::enable_if_t<!has_addpoints<T>::value>* = nullptr>
        void addpoints_(size_t beg, size_t end) {
            raise<LogicError>("NFKD: static index can not have points added");
            return; // no-op
        }

    };

}

#endif
