#ifndef WIRECELLUTIL_INDEXEDMULTISET
#define WIRECELLUTIL_INDEXEDMULTISET

#include <unordered_map>
#include <vector>
#include <algorithm>

namespace WireCell {

    /** IndexedMultiset - index a multiset
     *
     * This class maintains a multiset - a set with possible duplicates.
     *
     * Once added, collections of elements in different ordering:
     *
     * - first-seen index :: number incremented each time a novel element is added.
     * 
     * - as-added order :: number giving the order each element is added.
     *
     * Elements must be hashable.
     */

    template <class TYPE>
    class IndexedMultiset {
       public:

        using size_type = size_t;

        // Map element to first-seen order
        using index_type = std::unordered_map<TYPE, size_type>;
        using iterator = typename index_type::iterator;

        // Map as-added order to element;
        using order_type = std::vector<iterator>;

        IndexedMultiset() = default;
        ~IndexedMultiset() = default;

        template<typename Sequence>
        IndexedMultiset(const Sequence& seq) {
            for (const auto& val : seq) {
                (*this)(val);
            };
        }
        template<typename iterator>
        IndexedMultiset(iterator beg, iterator end) {
            while (beg != end) {
                (*this)(*beg);
                ++beg;
            }
        }

        const index_type& index() const { return m_index; };
        const order_type& order() const { return m_order; };

        // Intern an object as element and return its index.
        size_t operator()(const TYPE& obj) {
            const auto& [mit, _] = m_index.insert(std::make_pair(obj, m_index.size()));
            m_order.push_back(mit);
            return mit->second;
        }

        // Fill output iterator with first-seen indices in as-added order.
        template<typename Out>
        Out index_order(Out out) const
        {
            for (const auto& one : m_order) {
                *out = one->second;
                ++out;
            }
            return out;
        }

        // Return the first-seen indices in as-added order as vector.
        std::vector<size_type> index_order() const
        {
            std::vector<size_type> ret(m_order.size());
            index_order(ret.begin());
            return ret;            
        }

        // Fill output iterator with elements in first-seen indexing.
        template<typename Out>
        Out invert(Out out) const {
            using Itr = std::pair<TYPE, size_t>;
            std::vector< Itr > tmp(m_index.begin(), m_index.end());
            std::sort(tmp.begin(), tmp.end(), [](const auto&a, const auto& b) {
                return a.second < b.second;
            });
            for (const auto& one : tmp) {
                *out = one.first;
                ++out;
            }
            return out;
        }

        // Return an inversion of the index to map from first-seen
        // index to element.
        std::vector<TYPE> invert() const {
            std::vector<TYPE> ret(m_index.size());
            invert(ret.begin());
            return ret;
        }            


        bool has(const TYPE& obj)
        {
            auto mit = m_index.find(obj);
            return mit != m_index.end();
        }

      private:
        index_type m_index;
        order_type m_order;

    };

}  // namespace WireCell

#endif
