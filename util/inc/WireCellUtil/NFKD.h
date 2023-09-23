// An iterator based interface to nanoflan.

#ifndef WIRECELLUTIL_NFKD
#define WIRECELLUTIL_NFKD

#include "WireCellUtil/nanoflann.hpp"
#include "WireCellUtil/DetectionIdiom.h"
#include "WireCellUtil/Exceptions.h"

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
    using DistanceL1 = nanoflann::metric_L1;
    using DistanceL2 = nanoflann::metric_L2;
    using DistanceL2Simple = nanoflann::metric_L2_Simple;    


    template<typename Iter,
             typename DistanceTraits = DistanceL2Simple,
             typename IndexTraits = IndexDynamic> 
    class Tree {

      public:
        // Iterator must yield a vector-like object with dimension
        // equal to that of the k-d tree.
        using iterator_type = Iter;
        // The point type itself must appear as a vector-like...
        using point_type = typename iterator_type::value_type;
        // ...iterable of elements
        using element_type = typename point_type::value_type;

        using self_type = Tree<Iter, DistanceTraits, IndexTraits>;
        using metric_type = typename DistanceTraits::template traits<element_type, self_type>::distance_t;
        using index_type = typename IndexTraits::template traits<metric_type, self_type>::index_t;

        using distance_type = typename metric_type::DistanceType;

        /// A result of a k-d query are associated pairs.  The first
        /// part of the pair gives the iterator to the input point in
        /// the point cloud.  The second gives the distance from the
        /// query point to the point at that index.
        using result_item = std::pair<iterator_type, distance_type>;
        using results_type = std::vector<result_item>;

        explicit Tree(size_t dimensionality)
            : m_index(dimensionality, *this)
        {
            spdlog::debug("NFKD: constructor dimensionality {}", dimensionality);
        }
        Tree(size_t dimensionality, iterator_type beg, iterator_type end)
            : m_ranges{ {0, IterRange{beg, end}} }
            , m_size(std::distance(beg, end))
            , m_index(dimensionality, *this)
        {
            spdlog::debug("NFKD: constructor dimensionality {} range size {}",
                          dimensionality, m_size);
        }

        // Number of points.
        size_t size() const {
            return m_size;
        }
        size_t point_calls() const {
            return m_point_calls;
        }
        
        // Append a range of points.
        virtual void append(iterator_type beg, iterator_type end)
        {
            const size_t oldsize = size();
            IterRange ir{beg,end};
            m_ranges[oldsize] = ir;
            const size_t adding = ir.size();
            m_size += adding;
            spdlog::debug("NFKD::append: oldsize={} adding={}", oldsize, adding);
            this->addn<index_type>(oldsize, adding);
        }

        virtual results_type knn(size_t kay, const point_type& query_point) const {
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
                ret.push_back(std::make_pair(at(indices[ind]), distances[ind]));
            }
            return ret;
        }

        virtual results_type radius(distance_type rad, const point_type& query_point) const {
            std::vector<nanoflann::ResultItem<size_t, element_type>> res;
            nanoflann::RadiusResultSet<element_type, size_t> rs(rad, res);
            m_index.findNeighbors(rs, query_point.data());

            results_type ret;
            for (const auto& [index, distance] : res) {
                ret.push_back(std::make_pair(at(index), distance));
            }
            return ret;
        }

        // nanoflann dataset adaptor API.
        inline size_t kdtree_get_point_count() const {
            spdlog::debug("NFKD: size={}", size());
            return size();
        }
        template <class BBOX>
        bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

        // Return the iterator at global index
        iterator_type at(size_t idx) const {
            for (const auto& one : m_ranges) {
                if (one.first <= idx and idx < one.first + one.second.size()) {
                    return one.second.begin() + idx - one.first;
                }
            }
            raise<IndexError>("NFKD::Tree index past end");
            return iterator_type(); // not reached
        }

        // unwantedly, nanoflan deals in indices
        inline element_type kdtree_get_pt(size_t idx, size_t dim) const {
            auto it = at(idx);
            const auto& pt = *it;
            const element_type val = pt.at(dim);
            ++m_point_calls;
            spdlog::debug("NFKD: get pt({},{})={}", idx,dim,val);
            return val;
        }

      private:
        struct IterRange {
            iterator_type b,e;
            size_t size() const { return std::distance(b,e); } // maybe cache for speed
            iterator_type begin() const { return b; }
            iterator_type end() const { return e; }
        };
        using cumulative_ranges = std::map<size_t, IterRange>;
        cumulative_ranges m_ranges{};
        size_t m_size{0};
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
                spdlog::debug("NFKD: addn({},{})", beg, n);
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
