#ifndef WIRECELL_POINTCLOUDDATASET
#define WIRECELL_POINTCLOUDDATASET

#include "WireCellUtil/PointCloudArray.h"

namespace WireCell::PointCloud {

    /** A set of arrays accessed by their name.  All arrays must be
        kept to the same number of elements (points).
    */
    class Dataset {
      public:

        using metadata_t = Configuration;

        /// A dataset is a glorified map from string to array.
        using store_t = std::map<std::string, Array>;
        using item_t = std::pair<std::string, Array>;

        // copy constructor
        Dataset(const Dataset& other) = default;
        // move constructor
        Dataset(Dataset&& other) = default;

        Dataset& operator=(const Dataset& other) = default;

        // Default constructor
        Dataset() = default;

        // Construct a dataset using an existing store.
        explicit Dataset(const store_t& store) {
            for (const auto& [n,a] : store) {
                add(n,a);
            }
        }

        /// Return the common number of elements along the major axis
        /// of the arrays.  This returns the size of the first
        /// dimension or zero if the dataset is empty.
        size_t size_major() const {
            if (m_store.empty()) {
                return 0;
            }
            return m_store.begin()->second.size_major();
        }
        
        /** Add the array to the dataset.  Return true if its name is
            new.  Throw if number of elments of array does not match
            others in the dataset.  This will copy the array.
        */
        bool add(const std::string& name, const Array& arr);

        /** Add the array to the dataset.  Return true if its name is
            new.  Throw if number of elments of array does not match
            others in the dataset.  This will move the array.
        */
        // bool add(const std::string& name, Array&& arr);

        /// Access the underlying store
        const store_t& store() const { return m_store; }

        /** Return a vector of "references wrappers" around the arrays
            matching the names.  If any name is not provided by this
            then an empty collection is returned.  References may be
            unusual to some so here are two usage examples:

            sel = ds.selection({"one","two"});
            const Array& one = sel[0];
            size_t num_in_two = sel[1].get().num_elements();
        */
        selection_t selection(const name_list_t& names) const;

        /** Return named array or empty if not found */
        const Array& get(const std::string& name) const {
            auto it = m_store.find(name);
            if (it == m_store.end()) {
                static Array empty;
                return empty;
            }
            return it->second;            
        }
        Array& get(const std::string& name) {
            auto it = m_store.find(name);
            if (it == m_store.end()) {
                static Array empty;
                return empty;
            }
            return it->second;            
        }


        // FIXME, add something that returns something that produces a
        // typed tuple of array elements given their common index.

        /// Return the sorted names of the arrays in this.
        using keys_t = std::vector<std::string>;
        keys_t keys() const {
            keys_t ret;
            for (const auto& [k,v] : m_store) {
                ret.push_back(k);
            }
            std::sort(ret.begin(), ret.end());
            return ret;
        }
        bool has(const std::string& key) const {
            for (const auto& [k,v] : m_store) {
                if (k == key) return true;
            }
            return false;
        }

        /// Return a Dataset like this one but filled with zeros to
        /// given number of elements along major axis.
        Dataset zeros_like(size_t nmaj);

        /// Append arrays in tail to this.
        void append(const Dataset& tail) {
            append(tail.store());
        }
        
        /// Append arrays in map to this.
        void append(const std::map<std::string, Array>& tail);

        /** Append zeros to all existing arrays adding nmaj elements
            to the major axis.  This is essentially calling append()
            with the result of zeros_like(nmaj) and thus it triggers
            the update hook.
        */
        void extend(size_t nmaj);

        /** Register a function to be called after a successful append
            is performed on this dataset.  Function is called with the
            right-open range.  Ie, "end" is one past.  NOTE: this is
            different than nanoflan which expects a closed range. 
        */
        using append_callback_f = std::function<void(size_t beg, size_t end)>;
        void register_append(append_callback_f ac)
        {
            m_append_callbacks.push_back(ac);
        }

        metadata_t& metadata() { return m_metadata; }
        const metadata_t& metadata() const { return m_metadata; }

      private:

        store_t m_store;
        std::vector<append_callback_f> m_append_callbacks;
        
        // Metadata is a passive carry.
        metadata_t m_metadata;

    };                          // Dataset
    
    // A "null" filter which incurs a copy.
    inline Dataset copy(const Dataset& ds) { return ds; }

    using DatasetRef = std::reference_wrapper<const Dataset>;



}

#endif
