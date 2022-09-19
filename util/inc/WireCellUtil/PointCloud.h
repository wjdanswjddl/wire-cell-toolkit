#ifndef WIRECELL_POINTCLOUD
#define WIRECELL_POINTCLOUD

#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/nanoflann.hpp"
#include "boost/core/span.hpp"
#include "boost/multi_array.hpp"
#include <set>
#include <map>
#include <string>
#include <vector>
#include <iterator>

namespace WireCell::PointCloud {


    // A dense array held in a typeless manner
    class Array {
        
      public:
        // For scalar, per-point values shape must be (n,) where where
        // n is number of points.  N-dimensional, per-point data has a
        // shape of (n, d1, ...) where d1 is the size of first
        // dimension, etc for any others.
        using shape_t = std::vector<size_t>;

        // The underlying data may be accessed as a typed, flattened
        // span.
        template<typename ElementType>
        using span_t = boost::span<ElementType>;

        // The underlying data may be accessed as a typed, Boost multi
        // array to allow indexing.
        template<typename ElementType, size_t NumDims>
        using indexed_t = boost::const_multi_array_ref<ElementType, NumDims>;

        // Store a point array given in flattened, row-major aka
        // C-order.  If share is true then no copy of elements is
        // done.  See assure_mutable().
        template<typename ElementType>
        explicit Array(ElementType* elements, shape_t shape, bool share)
        {
            assign(elements, shape, share);
        }
        template<typename ElementType>
        explicit Array(const ElementType* elements, shape_t shape, bool share)
        {
            assign(elements, shape, share);
        }

        // Want defaults for all the rest.
        Array() = default;
        Array(const Array&) = default;
        Array& operator=(const Array&) = default;
        Array(Array&&) = default;
        Array& operator=(Array&&) = default;
        ~Array() = default;

        // Discard any held data and assign new data.  See constructor
        // for arguments.
        template<typename ElementType>
        void assign(ElementType* elements, shape_t shape, bool share)
        {
            clear();
            m_shape = shape;
            size_t nbytes = m_ele_size = sizeof(ElementType);
            for (const auto& s : m_shape) {
                nbytes *= s;
            }
            std::byte* bytes = reinterpret_cast<std::byte*>(elements);
            if (share) {
                m_span = span_t<std::byte>(bytes, nbytes);
            }
            else {
                m_store.assign(bytes, bytes+nbytes);
                update_span();
            }
        }

        /// Assign via typed pointer.
        template<typename ElementType>
        void assign(const ElementType* elements, shape_t shape, bool share) {
            assign((ElementType*)elements, shape, share);
        }

        // Clear all held data.
        void clear()
        {
            m_store.clear();
            m_span = span_t<std::byte>();
            m_shape.clear();
            m_ele_size = 0;
        }

        // Assure we are in mutable mode.  If previously sharing user
        // data, this results in a copy.
        void assure_mutable()
        {
            if (m_store.empty()) {
                m_store.assign(m_span.begin(), m_span.end());
                update_span();
            }
        }

        // Return object that allows type-full indexing of N-D array
        // (a Boost const multi array reference).  Note, user must
        // assure type is consistent with the originally provided data
        // at least in terms of element size.  NumDims must be the
        // same as the size of the shape() vector.
        template<typename ElementType, size_t NumDims>
        indexed_t<ElementType, NumDims> indexed() const
        {
            return indexed_t<ElementType, NumDims>(elements<const ElementType>().data(), m_shape);
        }

        // Return a constant span of flattened array data assuming
        // data elements are of given type.
        template<typename ElementType>
        span_t<const ElementType> elements() const
        {
            const ElementType* edata = 
                reinterpret_cast<const ElementType*>(m_span.data());
            return span_t<const ElementType>(edata, m_span.size()/sizeof(ElementType));
        }

        // Return element at index as type, no bounds checking is
        // done.
        template<typename ElementType>
        ElementType element(size_t index)
        {
            return *(reinterpret_cast<const ElementType*>(m_span.data()) + index);
        }

        // Return constant span array as flattened array as bytes.
        span_t<const std::byte> bytes() const
        {
            return m_span;
        }

        // Append a flat array of bytes.
        void append(const std::byte* data, size_t nbytes)
        {
            assure_mutable();
            m_store.insert(m_store.end(), data, data+nbytes);
            update_span();
        }

        template<typename ElementType>
        void append(const ElementType* data, size_t nele) 
        {
            append(reinterpret_cast<const std::byte*>(data),
                   nele * sizeof(ElementType));
        }

        template<typename Itr>
        void append(Itr beg, Itr end)
        {
            const size_t size = std::distance(beg, end);
            append(&*beg, size);
        }
        
        void append(const Array& arr)
        {
            append(arr.bytes());
        }

        template<typename Range>
        void append(const Range& r)
        {
            append(std::begin(r), std::end(r));
        }

        // Return the number of elements assuming original datasize.
        size_t num_elements() const {
            if (m_ele_size == 0) {
                return 0;
            }
            return m_span.size() / m_ele_size;
        }

      private:

        shape_t m_shape{};
        size_t m_ele_size{0};

        // if sharing user data, m_store is empty.
        std::vector<std::byte> m_store{};
        // view of either user's data or our store and through which
        // all access is done.
        span_t<std::byte> m_span{};

        void update_span() {
            m_span = span_t<std::byte>(m_store.data(), m_store.data() + m_store.size());
        }

    };

    using ArrayRef = std::reference_wrapper<const Array>;

    /// A set of arrays accessed by their name.  All arrays must be
    /// kept to the same number of elements (points).
    class Dataset {
      public:

        /// Return the number of elements in the arrays of the dataset.
        size_t num_elements() const {
            if (m_store.empty()) {
                return 0;
            }
            return m_store.begin()->second.num_elements();
        }
        
        // Add the array to the dataset.  Return true if its name is
        // new.  Throw if number of elments of array does not match
        // others in the dataset.  This will copy the array.
        bool add(const std::string& name, const Array& arr);

        // Add the array to the dataset.  Return true if its name is
        // new.  Throw if number of elments of array does not match
        // others in the dataset.  This will move the array.
        bool add(const std::string& name, Array&& arr);

        // A dataset is a glorified map from string to array.
        using store_t = std::map<std::string, Array>;
        using item_t = std::pair<std::string, Array>;
        struct store_cmp {
            bool operator()(const item_t& a, const item_t & b) const {
                return a.first < b.first;
            }
        };

        const store_t& store() const { return m_store; }


        // A selection of arrays, held by array ref so updates are
        // reflected.
        using selection_t = std::vector<ArrayRef>;

        // Return a vector of references to arrays matching names.  If
        // any name is not provided by this then an empty collection
        // is returned.
        selection_t selection(const std::vector<std::string>& names) const;


        // FIXME, add something that returns something that produces a
        // typed tuple of array elements given their common index.

        using keys_t = std::set<std::string>;
        keys_t keys() const {
            keys_t ret;
            for (const auto& [k,v] : m_store) {
                ret.insert(k);
            }
            return ret;
        }

        // Return the key names in this dataset which are missing from
        // the other dataset.
        keys_t missing(const Dataset& other);

        /// Append arrays in tail to this.
        void append(const Dataset& tail) {
            append(tail.store());
        }
        
        /// Append arrays in map to this.
        void append(const std::map<std::string, Array>& tail);

      private:

        store_t m_store;
        
    };                          // Dataset
    
    // This class may be used as a "dataset adaptor" for a nanoflan
    // point cloud.
    template<typename ElementType>
    class NanoflannAdaptor {
        Dataset::selection_t m_pts;
      public:
        using element_t = ElementType;

        // Construct with a reference to a dataset and the ordered
        // list of arrays in the dataset to use as the coordinates.
        NanoflannAdaptor(const Dataset::selection_t& points)
            : m_pts(points)
        {
        }

        inline size_t kdtree_get_point_count() const
        {
            if (m_pts.empty()) {
                return 0;
            }
            const ElementType& first = m_pts[0];
            return first.num_elements();
        }

        inline element_t kdtree_get_pt(size_t idx, size_t dim) const
        {
            if (dim < m_pts.size()) {
                const Array& arr = m_pts[dim];
                if (idx < arr.num_elements()) {
                    return arr.element<ElementType>(idx);
                }
            }
            THROW(IndexError() << errmsg{"index out of bounds"});
        }
    };


}

#endif
