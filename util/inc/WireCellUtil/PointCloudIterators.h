#ifndef WIRECELLUTIL_POINTCLOUDCOORDINATES
#define WIRECELLUTIL_POINTCLOUDCOORDINATES

#include "WireCellUtil/PointCloudArray.h"

#include <boost/iterator/iterator_adaptor.hpp>

namespace WireCell::PointCloud {

    // Iterate on a PointCloud::selection_t assuming the arrays are
    // homotypical.  
    template<typename VectorType>
    class coordinate_iterator
        : public boost::iterator_facade<coordinate_iterator<VectorType>
                                        , VectorType
                                        , boost::random_access_traversal_tag
                                        >
    {
      public:
        using base_type = boost::iterator_facade<coordinate_iterator<VectorType>
                                                 , VectorType
                                                 , boost::random_access_traversal_tag
                                                 >;
        using self_type = coordinate_iterator<VectorType>;
        using difference_type = typename base_type::difference_type;
        using value_type = typename base_type::value_type;
        using pointer = typename base_type::value_type*;
        using reference = typename base_type::value_type&;

        using element_type = typename VectorType::value_type;

        coordinate_iterator() = default; // "end" iterator

        // Construct with copy of value
        coordinate_iterator(selection_t& selection,
                            const VectorType& value)
            : m_value(value)
        {
            init(selection);
        }
        // Construct value with dimension from selection size.
        explicit coordinate_iterator(selection_t& selection)
            : m_value(selection.size())
        {
            init(selection);
        }

      private:
        friend class boost::iterator_core_access;

        using span_type = typename Array::span_t<const element_type>;
        using span_vector = std::vector<span_type>;

        size_t index{0};
        span_vector spans{};
        mutable value_type m_value{}; // vector like

        void init(selection_t& selection) {
            for (const Array& array : selection) {
                span_type span = array.elements<element_type>();
                spans.push_back(span);
            }
        }

        size_t size() const {
            if (spans.empty()) { return 0; }
            return spans[0].size();
        }

        bool at_end() const {
            return index == size();
        }

        template <typename OtherType>
        bool equal(const coordinate_iterator<OtherType> & x) const {
            if (at_end() and x.at_end()) return true;

            // This is weak.  To be strong we must hold and check
            // identity of underlying arrays.  Implicitly, this and x
            // must be iterating on a selection of the same arrays.
            return index == x.index and size() == x.size();
        }

        void increment () { ++index; }
        void decrement () { --index; }
        void advance (difference_type n) { index += n; }

        difference_type
        distance_to(self_type const& other) const
        {
            return other.index - this->index;
        }

        // template<typename V, std::enable_if_t<std::is_arithmetic_v<std::decay_t<V>>, bool> = true>
        // V do_reference() const {
        //     return spans[0][index];
        // }
        // template<typename V, std::enable_if_t<!std::is_arithmetic_v<std::decay_t<V>>, bool> = true>
        // V do_reference() const {
        // }
        

        reference dereference() const
        {
            const size_t ndim = spans.size();
            m_value.resize(ndim);
            for (size_t ind=0; ind<ndim; ++ind) {
                m_value[ind] = spans[ind][index];
            }
            return m_value;
        }
    };

    template<typename VectorType>
    struct coordinates {
        using iterator = coordinate_iterator<VectorType>;
        using value_type = typename coordinate_iterator<VectorType>::value_type;
        iterator b,e;
        coordinates(selection_t& selection)
            : b(selection), e() {};
        coordinates(selection_t& selection, const VectorType& point)
            : b(selection, point), e() {};
        iterator begin() const { return b; }
        iterator end() const { return e; }
    };

}

#endif


