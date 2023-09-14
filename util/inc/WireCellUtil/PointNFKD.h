// Adaptor to nanoflann over a disjoint dataset

#ifndef WIRECELLUTIL_POINTNFKD
#define WIRECELLUTIL_POINTNFKD

#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/DetectionIdiom.h"
#include "WireCellUtil/nanoflann.hpp"

namespace WireCell::PointCloud::Tree {

    /// Base class for interface to nanoflann dataset and index query
    /// adaptors.  This is intended to be constructed via a Points and
    /// not directly by application code.
    class NFKD {
      protected:
        DisjointDataset m_dds;
      public:
        
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

        /// Update the disjoint dataset and refresh the KD tree
        void update(Dataset& newds)
        {
            size_t beg = m_dds.npoints();
            m_dds.append(newds);
            size_t end = m_dds.npoints();
            this->addpoints(beg, end);
        }
        // nanoflann dataset adaptor API (one more in KDQueryTyped).
        inline size_t kdtree_get_point_count() const {
            return m_dds.npoints();
        }
        template <class BBOX>
        bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

      protected:
        virtual void addpoints(size_t beg, size_t end) = 0;

    };

    /// Typed interface to nanoflann
    ///
    /// - ElementType provides the numeric type (eg, int, double) of the coordinate arrays.
    ///
    /// - DistanceTraits provides a .traits template giving the type for the distance metric.  Eg nanoflann::metric_L2.
    ///
    /// - IndexTraits provides a .traits template giving the type for the index adaptor.  Eg IndexStatic.
    ///
    /// FIXME: this may break if points are subsequently added to any DS.
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
        using metric_t = typename DistanceTraits::template traits<element_t, dataset_t>;
        // Derive the k-d tree index type.
        using index_t = typename IndexTraits::template traits<metric_t, dataset_t>;

        NFKDT() = delete;

        /// Construct with a dataset and a list of dataset attribute
        /// array names to interpret as coordinates.  These arrays
        /// must be typed consistent with ElementType.
        explicit NFKDT(const DisjointDataset& dds, const name_list_t& coords)
            : m_dds(dds)
            , m_coords(coords)
            , m_index(dds.npoints(), *this)
        {
            // fixme: check coords types?
        }


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
                ret.insert(indices[ind], distances[ind]);
            }
            return ret;
        }

        /** Return the neighbor points within a metric distance d from query point.

            IMPORTANT NOTE: the units in which r is expressed must
            match the metric.  If using an L2 metric, r is in units of
            [length^2].
        */
        virtual results_t radius(distance_t rad, const point_t& query_point) const {
            // nanoflann API change
            // std::vector<std::pair<size_t, element_t>> ids;
            std::vector<nanoflann::ResultItem<size_t, element_t>> ids;
            nanoflann::RadiusResultSet<element_t, size_t> rs(rad, ids);
            m_index.findNeighbors(rs, query_point.data(),
                                  nanoflann::SearchParameters());

            results_t ret;
            for (const auto& [index, distance] : ids) {
                ret.insert(index, distance);
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
            auto [dind,lind] = m_dds.index(idx);
            const Dataset& ds = m_dds.datasets()[dind];
            const auto& name = m_coords[dim];
            return ds.get(name).element<element_t>(lind);
        }

      protected:

        // This is called by parent class when ds is updated.
        virtual void addpoints(size_t beg, size_t end)
        {
            addpoints_<index_t>(beg,end);
        }

      private:
        DisjointDataset m_dds;
        index_t m_index;
        name_list_t m_coords;

        // discovery idiom so we only try to call addPoints if our index is dynamic.
        template <typename T, typename ...Ts>
        using addpoints_type = decltype(std::declval<T>().addPoints(std::declval<Ts>()...));

        template<typename T>
        using has_addpoints = is_detected<addpoints_type, T, uint32_t, uint32_t>;

        // Dynamic index case
        template <class T, std::enable_if_t<has_addpoints<T>::value>* = nullptr>
        void addpoints_(size_t beg, size_t end) const {
            if (end > beg) {
                this->m_index.addPoints(beg, end-1);
            }
        }

        // Static index case
        template <class T, std::enable_if_t<!has_addpoints<T>::value>* = nullptr>
        void addpoints_(size_t beg, size_t end) const {
            // fixme: should we throw here?
            return; // no-op
        }

    };

}

#endif
