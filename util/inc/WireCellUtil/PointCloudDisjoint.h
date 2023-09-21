#ifndef WIRECELLUTIL_POINTCLOUDDISJOINT
#define WIRECELLUTIL_POINTCLOUDDISJOINT

#include "WireCellUtil/Disjoint.h"
#include "WireCellUtil/PointCloudDataset.h"

namespace WireCell::PointCloud {

    // An array of Array
    class DisjointArray : public Disjoint<Array> {
      public:

        template<typename Numeric>
        Numeric element(const address_t& addr) const
        {
            const Array& arr = m_values.at(addr.first);
            return arr.element<Numeric>(addr.second());
        }

        template<typename Numeric>
        Numeric element(size_t index) const
        {
            auto addr = address(index);
            return element<Numeric>(addr);
        }
    };


    /// A light-weight sequence of references to individual PC
    /// datasets.
    ///
    /// Caller must keep Datasets alive for the lifetime of the set.
    ///
    class DisjointDataset : public Disjoint<Dataset> {
      public:
        
        // Return a disjoint arrays cooresponding to the named arrays.
        std::vector<DisjointArray> selection(const name_list_t& names);


        // // Return 
        // template<typename Numeric>
        // std::vector<Numeric> element(const address_t& addr) {
            
        // }
        // template<typename Numeric>
        // std::vector<Numeric> element(size_t index) {
        //     auto addr = address(index);
        //     return element(addr);
        // }

        // Append with registration
        void append_react(Dataset& ds) {
            // If DS changes, we invalidate any indexing
            ds.register_append([this](size_t beg, size_t end) {
                this->m_dirty = true;
            });
            append(ds);
        }

      private:

    };

}

#endif
