/**
   Aggregate disjoint PointCloud::Datasets 
 */

#ifndef WIRECELLUTIL_POINTCLOUDDISJOINT
#define WIRECELLUTIL_POINTCLOUDDISJOINT

#include "WireCellUtil/Disjoint.h"
#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/PointCloudCoordinates.h"

#include "WireCellUtil/Logging.h" // debug

namespace WireCell::PointCloud {

    // A point cloud disjoint is a collection of references to PC
    // arrays or datasets that look like a single collection.

    // Base class for DisjointArray and DisjointDataset
    template<typename Value>
    class DisjointBase {
      public:
        using value_type = Value;
        using reference_t = std::reference_wrapper<const value_type>;
        using reference_vector = std::vector<reference_t>;
        using address_t = std::pair<size_t,size_t>;

        const reference_vector& values() const { return m_values; }
        reference_vector& values() { return m_values; }

        // The number of elements, sum of size_major().
        const size_t size() const {
            update();
            return m_nelements;
        }

        address_t address(size_t index) const {
            // fixme: this algorithm likely can be optimized.
            const auto nvals = m_values.size();
            for (size_t valind=0; valind < nvals; ++valind) {
                const value_type& val = m_values[valind];
                const size_t vsize = val.size_major();
                if (index < vsize) {
                    return std::make_pair(valind, index);
                }
                index -= vsize;
            }
            raise<IndexError>("DisjointBase::index out of bounds");
            return std::make_pair(-1, -1); // not reached, make compiler happy
        }

        void append(const Value& val) {
            m_values.push_back(std::cref(val));
            spdlog::debug("DisjointBase: append size={}", m_nelements);
            if (m_dirty) {
                return;
            }
            // if clean, save some time, directly update
            m_nelements += val.size_major();
        }

        void update() const {
            if (m_dirty) {
                m_nelements = 0;
                for (const Value& val : m_values) {
                    m_nelements += val.size_major();
                }
                m_dirty = false;
                spdlog::debug("DisjointBase: update size={}", m_nelements);
            }
        }
        bool operator==(DisjointBase const& other) const {
            const size_t nparts = m_values.size();
            if (nparts != other.m_values.size()) {
                return false;
            }
            for (size_t ind=0; ind < nparts; ++ind) {
                const Value& a = m_values[ind];
                const Value& b = m_values[ind];
                if (a == b) { continue; }
                spdlog::debug("DisjointBase: datasets differ at index {}", ind);
                return false;
            }
            return true;            
        }
        bool operator!=(DisjointBase const& other) const {
            return !(*this == other);
        }

      protected:
        reference_vector m_values;
        mutable size_t m_nelements{0};
        mutable bool m_dirty{false};

    };

    // An array of Array
    // template<typename ElementType>
    // class DisjointArray : public DisjointBase<Array> {
    //   public:

    //     ElementType element(const address_t& addr) const
    //     {
    //         const Array& arr = m_values.at(addr.first);
    //         return arr.element(addr.second());
    //     }

    //     ElementType element(size_t index) const
    //     {
    //         auto addr = address(index);
    //         return element(addr);
    //     }

    //     using span_type = typename Array::span_t<ElementType>;
    //     using span_vector = typename std::vector<Array::span_t<ElementType>>;
    //     mutable span_vector spans;

    //     using iterator = disjoint<typename span_vector::iterator>;
            
    //     void update_() const
    //     {
    //         if (spans.size() == m_values.size()) {
    //             return;
    //         }
    //         spans.clear();
    //         for (const Array& array : m_values) {
    //             spans.push_back(array.elements<ElementType>());
    //         }
    //     }
    //     iterator begin() const {
    //         update_();
    //         return iterator(spans.begin(), spans.end());
    //     }
    //     iterator end() const {
    //         update_();
    //         return iterator(spans.end());
    //     }
    // };


    template<typename ElementType>
    struct disjoint_selection;

    /// A disjoint dataset.
    ///
    class DisjointDataset : public DisjointBase<Dataset> {
      public:
        
        
        template<typename VectorType>
        disjoint_selection<VectorType>
        selection(const name_list_t& names) const {
            return disjoint_selection<VectorType>(*this, names);
        }

    };

    /** The disjoint selection is a place to hold the selection and
     *  dispense coordinate iterators on the selection. 
     */
    template<typename VectorType>
    class disjoint_selection {
      public:

        // The coordinate vector type
        using vector_type = VectorType;
        // The type of one coordinate
        // using element_type = typename vector_type::value_type;

        // column-wise iterator on homotypic selection
        using coordinates_type = coordinates<VectorType>;
        // type to store those ranges
        using disjoint_coordinates = std::vector<coordinates_type>;
        // flat iterator on above
        using disjoint_type = disjoint<typename disjoint_coordinates::iterator>;
        using iterator = disjoint_type;

        disjoint_selection() = default;
        disjoint_selection(const disjoint_selection&) = default;
        disjoint_selection(disjoint_selection&&) = default;
        disjoint_selection& operator=(const disjoint_selection&) = default;
        disjoint_selection& operator=(disjoint_selection&&) = default;

        // construct a disjoint selection on a dataset and a list of
        // attribute array names.
        disjoint_selection(DisjointDataset dj, const name_list_t& names)
            : m_names(names)
        {
            for (const Dataset& ds : dj.values()) {
                auto sel = ds.selection(names);
                m_store.push_back(coordinates<VectorType>(sel));
            }
            m_djc = flatten(m_store);
        }
            
        size_t size() const { return m_djc.size(); }

        // Range API
        iterator begin() const {
            return m_djc.begin();
        }
        iterator end() const {
            return m_djc.end();
        }

        void append(const Dataset& ds) {
            auto sel = ds.selection(m_names);
            m_store.push_back(coordinates<VectorType>(sel));
            auto it = m_store.end();
            --it;
            m_djc.append(it);
        }


      private:
        name_list_t m_names;
        disjoint_type m_djc;
        disjoint_coordinates m_store;
     };
}

#endif
