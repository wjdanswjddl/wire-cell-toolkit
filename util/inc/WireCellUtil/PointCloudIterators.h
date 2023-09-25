#ifndef WIRECELLUTIL_POINTCLOUDCOORDINATES
#define WIRECELLUTIL_POINTCLOUDCOORDINATES

#include "WireCellUtil/PointCloudArray.h"
#include "WireCellUtil/Logging.h"

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
            spdlog::debug("coordinate_iterator construct with value of size {}", m_value.size());
            init(selection);
        }
        // Construct value with dimension from selection size.
        explicit coordinate_iterator(selection_t& selection)
            : m_value(selection.size())
        {
            spdlog::debug("coordinate_iterator construct with size {}", m_value.size());
            init(selection);
        }

        // The size of the vector-like value
        size_t ndim() const {
            return m_value.size();
        }

        // The size of each of the spans
        size_t size() const {
            if (m_spans.empty()) { return 0; }
            return m_spans[0].size();
        }

      private:
        friend class boost::iterator_core_access;

        using span_type = typename Array::span_t<const element_type>;
        using span_vector = std::vector<span_type>;

        size_t m_index{0};            // index in the span
        span_vector m_spans{};        // one span per dimension
        mutable value_type m_value{}; // vector like

        void init(selection_t& selection) {
            for (const Array& array : selection) {
                span_type span = array.elements<element_type>();
                m_spans.push_back(span);
            }
            spdlog::debug("coordinate_iterator init with dimension {} and {} spans of size {}",
                          m_value.size(), m_spans.size(), size());
        }

        bool at_end() const {
            return m_index == size();
        }

        template <typename OtherType>
        bool equal(const coordinate_iterator<OtherType> & x) const {
            if (at_end() and x.at_end()) return true;

            // This is weak.  To be strong we must hold and check
            // identity of underlying arrays.  Implicitly, this and x
            // must be iterating on a selection of the same arrays.
            return m_index == x.m_index and size() == x.size();
        }

        void increment () {
            spdlog::debug("coordinate_iterator::increment index={}/{}", m_index, size());
            ++m_index;
        }
        void decrement () { --m_index; }
        void advance (difference_type n) {
            spdlog::debug("coordinate_iterator::advance index={}/{} by {}", m_index, size(), n);
            m_index += n;
        }

        difference_type
        distance_to(self_type const& other) const
        {
            // fixme: what if this or other is end?
            spdlog::debug("coordinate_iterator::distance_to: {} - {} = {}, size={}, ndim={}",
                          other.m_index, this->m_index, other.m_index - this->m_index,
                          size(), ndim());
            return other.m_index - this->m_index;
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
            const size_t ndims = ndim();
            // m_value.resize(ndim);
            spdlog::debug("coordinate_iterator::dereference {} dimensions at index={}/{}",
                          ndims, m_index, size());
            for (size_t ind=0; ind<ndims; ++ind) {
                m_value[ind] = m_spans[ind][m_index];
                spdlog::debug("\t{}: {}", ind, m_value[ind]);
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


