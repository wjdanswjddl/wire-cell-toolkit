#ifndef WIRECELLUTIL_DISJOINT
#define WIRECELLUTIL_DISJOINT

#include "WireCellUtil/Exceptions.h"

#include <functional>           // reference_wrapper
#include <vector>
#include <map>                  // pair

namespace WireCell {

    // https://stackoverflow.com/a/11791223
    template<typename Iter>
    class disjoint_iterator
        : public boost::iterator_adaptor<
        disjoint_iterator<Iter>,
        Iter,
        typename Iter::value_type::iterator::value_type,
        boost::forward_traversal_tag, // fixme: upgrade to random traversal
        typename Iter::value_type::iterator::value_type        
        >
    {
      private:
        using super_t = boost::iterator_adaptor<
          disjoint_iterator<Iter>,
          Iter,
          typename Iter::value_type::iterator::value_type,
          boost::forward_traversal_tag, // fixme: upgrade to random traversal
          typename Iter::value_type::iterator::value_type        
          >;
        using inner_iterator = typename Iter::value_type::iterator;

      public:


        disjoint_iterator(Iter it)
            : super_t(it),
              inner_begin(),
              inner_end(),
              outer_end(it)
        {
        }
        disjoint_iterator(Iter begin, Iter end)
            : super_t(begin),
              inner_begin((*begin).begin()),
              inner_end((*begin).end()),
              outer_end(end)
        {
        }

        using value_type = typename Iter::value_type::iterator::value_type;
      private:
        friend class boost::iterator_core_access;
        inner_iterator inner_begin;
        inner_iterator inner_end;
        Iter outer_end;

        void increment()
        {
            if (this->base_reference() == outer_end)
                return; // At the end

            ++inner_begin;
            if (inner_begin == inner_end)
            {
                ++this->base_reference();
                inner_begin = (*this->base_reference()).begin();
                inner_end = (*this->base_reference()).end();
            }
        }

        value_type dereference() const
        {
            return *inner_begin;
        }

    };

    template<typename Iter>
    struct disjoint_range {
        using iterator = disjoint_iterator<Iter>;

        Iter b, e;
        iterator begin() const { return iterator(b,e); }
        iterator end() const { return iterator(e); }
    };

    template<typename Collection>
    disjoint_range<typename Collection::iterator> disjoint(Collection& col) {
        return disjoint_range<typename Collection::iterator>{col.begin(), col.end()};
    }

    template<typename Value>
    class Disjoint {
      public:
        using value_type = Value;
        using reference_t = std::reference_wrapper<const value_type>;
        using reference_vector = std::vector<reference_t>;
        using address_t = std::pair<size_t,size_t>;

        const reference_vector& values() const { return m_values; }
        reference_vector& values() { return m_values; }

        const size_t nelements() const {
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
            raise<IndexError>("Disjoint::index out of bounds");
            return std::make_pair(-1, -1); // not reached, make compiler happy
        }

        void append(const Value& val) {
            m_values.push_back(std::cref(val));
            // if already clean, save some time and stay clean
            if (!m_dirty) {
                m_nelements += val.size_major();
            }
        }

        void update() const {
            if (!m_dirty) return;

            m_nelements = 0;
            for (const Value& val : m_values) {
                m_nelements += val.size_major();
            }
            m_dirty = false;
        }

      protected:
        reference_vector m_values;
        mutable size_t m_nelements{0};
        mutable bool m_dirty{true};

    };
}

#endif
