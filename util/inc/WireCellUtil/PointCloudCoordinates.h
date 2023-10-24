#ifndef WIRECELLUTIL_POINTCLOUDCOORDINATES
#define WIRECELLUTIL_POINTCLOUDCOORDINATES

#include "WireCellUtil/PointCloudArray.h"

#include <boost/iterator/iterator_adaptor.hpp>
#include <memory>

namespace WireCell::PointCloud {

    // fixme: rename file to PointCloudCoordinates

    // An iterator on a PointCloud::selection_t that returns a
    // vector-like value.
    template<typename VectorType>
    class coordinates
        : public boost::iterator_facade<coordinates<VectorType>
                                        , VectorType
                                        , boost::random_access_traversal_tag
                                        >
    {
      public:
        using base_type = boost::iterator_facade<coordinates<VectorType>
                                                 , VectorType
                                                 , boost::random_access_traversal_tag
                                                 >;
        using self_type = coordinates<VectorType>;
        using iterator = coordinates<VectorType>;
        using difference_type = typename base_type::difference_type;
        using value_type = typename base_type::value_type;
        using pointer = typename base_type::value_type*;
        using reference = typename base_type::value_type&;

        using element_type = typename VectorType::value_type;

        coordinates()
        {
        }

        coordinates(const coordinates&) = default;
        coordinates(coordinates&&) = default;
        coordinates& operator=(const coordinates&) = default;
        coordinates& operator=(coordinates&&) = default;

        // Construct with copy of value
        coordinates(const selection_t& selection, const VectorType& value)
            : m_value(value)
        {
            init(selection);
        }
        // Construct value with dimension from selection size.
        explicit coordinates(const selection_t& selection)
            : m_value(selection.size())
        {
            init(selection);
        }

        iterator begin() const {
            return *this;
        }
        iterator end() const {
            auto it = *this;
            it.m_index = it.size();
            return it;
        }

        // The size of the vector-like value
        size_t ndim() const {
            return m_value.size();
        }

        // The size of each of the spans
        size_t size() const {
            if (! m_spansp) return 0;
            if (m_spansp->empty()) return 0;
            //m_spans.empty()) { return 0; }
            // return m_spans[0].size();
            return m_spansp->at(0).size();
        }

      private:
        friend class boost::iterator_core_access;

        using span_type = typename Array::span_t<const element_type>;
        using span_vector = std::vector<span_type>;

        // index in the span
        size_t m_index{0};
        // one span per dimension.  It is a little weird to use a
        // shared pointer here but it reduces the cost of a copy.  
        std::shared_ptr<span_vector> m_spansp{};
        // we must hold m_value so we may return a reference.
        mutable value_type m_value{}; 

        void init(const selection_t& selection) {
            m_spansp = std::make_shared<span_vector>();
            for (const Array& array : selection) {
                span_type span = array.elements<element_type>();
                // m_spans.push_back(span);
                m_spansp->push_back(span);
            }
        }

        bool at_end() const {
            return m_index == size();
        }

        template <typename OtherType>
        bool equal(const coordinates<OtherType> & x) const {
            return m_index == x.m_index;
        }

        void increment () {
            ++m_index;
        }
        void decrement () { --m_index; }
        void advance (difference_type n) {
            m_index += n;
        }

        difference_type
        distance_to(self_type const& other) const
        {
            return other.m_index - this->m_index;
        }

        reference dereference() const
        {
            const size_t ndims = ndim();
            for (size_t ind=0; ind<ndims; ++ind) {
                // m_value[ind] = m_spans[ind][m_index];
                m_value[ind] = m_spansp->at(ind)[m_index];
            }
            return m_value;
        }
    };

}

#endif


