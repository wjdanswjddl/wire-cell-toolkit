#ifndef WIRECELLUTIL_INDEXEDMULTISET
#define WIRECELLUTIL_INDEXEDMULTISET

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <functional>

namespace WireCell {

    /** IndexedMultiset - index a multiset
     *
     * This class maintains a multiset - a set with possible duplicates.
     *
     * The IndexedMultiset provides two collections:
     *
     * - index :: a zero-based sequential count for the value.  Every
     *   insert of a value returns the same index.  The index is
     *   simply the number of elements previously seen prior to the
     *   insertion of the value for the first time.  The size of the
     *   index is the number of unique values that have been inserted.
     *
     * - order :: maintains the order of insertion.  Elements of this
     *   collection are elements of the index collection and thus
     *   duplicates may exist.  The size of the order is the number of
     *   insertions.
     *
     * Inserted values must be hashable and comparable.
     */
    template <class TYPE, class Count = size_t, class Hash = std::hash<TYPE>>
    class IndexedMultiset {
       public:

        using size_type = Count;
        using hash_type = Hash;

        // Map element to first-seen order
        using index_type = std::unordered_map<TYPE, size_type, hash_type>;
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

        // Access the index collection holding a map from unique value
        // to its sequential index number.
        const index_type& index() const { return m_index; };
        // Access the order collection with each element corresponding
        // to an inserted value and gives an iterator into the index.
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

        // Return vector of indices corresponding to the sequence of
        // inserted values.
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

        // Return an inversion of the index.  The vector holds the
        // unique inserted values in order of their index.
        std::vector<TYPE> invert() const {
            std::vector<TYPE> ret(m_index.size());
            invert(ret.begin());
            return ret;
        }            

        // Return true if we have hte object.
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
