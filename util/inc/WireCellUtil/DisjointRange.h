#ifndef WIRECELLUTIL_DISJOINTRANGE
#define WIRECELLUTIL_DISJOINTRANGE

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range.hpp>
#include <map>
#include <stdexcept>

namespace WireCell {

    /** A collection of iterator ranges presented as a flat collection of iterators.

        The disjoint_range defines the following iterator types:

        - minor iterator :: iterates within each iterator range.

        - major iterator :: iterates over the iterator ranges themselves.

        - iterator :: aka disjoint cursor.  It yields a pair holding
          major and minor iterators.
     */

    template<typename MajorIter, typename MinorIter, typename MinorValue>
    class disjoint_cursor;

    template<typename UserMinorRangeType>
    class disjoint_range {
      public:
        using self_type = disjoint_range<UserMinorRangeType>;

        // using minor_range = UserMinorRangeType;
        using minor_range = boost::sub_range<UserMinorRangeType>;
        using minor_iterator = typename minor_range::iterator;
        using const_minor_iterator = typename minor_range::const_iterator;
        using value_type = typename minor_iterator::value_type;
        using const_value_type = typename minor_iterator::value_type const;

        // We store ranges of minor iterators with a key giving the
        // accumulated number of minor iterators prior to each item.
        using major_map = std::map<size_t, minor_range>;
        using major_iterator = typename major_map::iterator;
        using const_major_iterator = typename major_map::const_iterator;

        // The flattened iterator
        using iterator = disjoint_cursor<major_iterator, minor_iterator, value_type>;
        using const_iterator = disjoint_cursor<const_major_iterator, const_minor_iterator, const_value_type>;


        disjoint_range()
            : last(accums.begin(), accums.end())
        {
        }

        template<typename Container>
        explicit disjoint_range(Container& con)
            : last(accums.begin(), accums.end())
        {
            fill(con);
        }

        template<typename RangeIter>
        explicit disjoint_range(RangeIter beg, RangeIter end)
            : last(accums.begin(), accums.end())
        {
            append(beg, end);
        }

        iterator begin()
        {
            if (accums.empty()) return end();
            return iterator(accums.begin(), accums.end());
        }

        iterator end()
        {
            return iterator(accums.end(), accums.end(), size_);
        }

        const_iterator begin() const
        {
            if (accums.empty()) return end();
            return const_iterator(accums.cbegin(), accums.cend());
        }

        const_iterator end() const
        {
            return const_iterator(accums.cend(), accums.cend(), size_);
        }

        template<typename Container>
        void fill(Container& con) {
            for (auto& r : con) {
                append(r);
            }
        }

        template<typename Range>
        void append(Range& r) {
            if (r.empty()) return;
            accums[size_] = minor_range(r);
            size_ += accums.rbegin()->second.size();
            last = begin();
        }
        template<typename RangeIter>
        void append(RangeIter ribeg, RangeIter riend) {
            if (ribeg == riend) return;
            accums[size_] = minor_range(ribeg, riend);
            size_ += accums.rbegin()->second.size();
            last = begin();
        }

        size_t size() const { return size_; }

        void clear() {
            accums.clear();
            size_ = 0;
            last = begin();
        }

        const value_type& operator[](size_t ind) const {
            last += ind - last.index();
            return *last;
        }
        value_type& operator[](size_t ind) {
            last += ind - last.index();
            return *last;
        }
        const value_type& at(size_t ind) const {
            if (ind < size()) {
                return (*this)[ind];
            }
            throw std::out_of_range("index out of range");
        }
        value_type& at(size_t ind) {
            if (ind < size()) {
                return (*this)[ind];
            }
            throw std::out_of_range("index out of range");
        }

      private:
            
        major_map accums;
        size_t size_{0};

        // cache to sometimes speed up random access
        mutable iterator last;  
    };

        
    template <typename MajorIter, typename MinorIter, typename MinorValue>
    class disjoint_cursor
        : public boost::iterator_facade<disjoint_cursor<MajorIter, MinorIter, MinorValue>
                                        // value
                                        , MinorValue
                                        // cagegory
                                        , boost::random_access_traversal_tag
                                        // reference
                                        , MinorValue&
                                        >
    {
      public:

        using major_iterator = MajorIter;
        using minor_iterator = MinorIter;

        // iterator facade types
        using base_type =
            boost::iterator_facade<disjoint_cursor<MajorIter, MinorIter, MinorValue>
                                   , MinorValue
                                   , boost::random_access_traversal_tag
                                   , MinorValue&
                                   >;
        using difference_type = typename base_type::difference_type;
        using value_type = typename base_type::value_type;
        using pointer = typename base_type::pointer;
        using reference = typename base_type::reference;

      private:
        major_iterator major_end;
        major_iterator major_; // accum->minor_range
        minor_iterator minor_;
        size_t index_{0};    // into the flattened iteration

      public:

        // To create an "end" cursor, pass b==e and index==size.
        // To create a "begin" cursor, pass b!=e and index==0.
        disjoint_cursor(major_iterator b, major_iterator e, size_t index=0)
            : major_end(e)
            , index_(index)
        {
            major_ = b;
            if (b == e) {
                // never deref minor_ if major_ == end!
                return;
            }
            minor_ = b->second.begin();
        }

        major_iterator major() const { return major_; }
        minor_iterator minor() const { return minor_; }
        size_t index() const { return index_; }

      private:
        friend class boost::iterator_core_access;

        bool equal(disjoint_cursor const& o) const {
            return
                // both are at end
                (major_ == major_end && o.major_ == o.major_end)
                or
                // same cursor location
                (major_ == o.major_ && minor_ == o.minor_);
        }

        void increment() {
            if (major_ == major_end) {
                throw std::out_of_range("increment beyond end");
            }
            ++minor_;
            if (minor_ == major_->second.end()) {
                ++major_;
                minor_ = major_->second.begin();
            }
            ++index_;
        }
        void decrement() {
            if (index_ == 0) {
                throw std::out_of_range("decrement beyond begin");
            }

            // if sitting just off end of major
            if (major_ == major_end) {
                --major_;        // back off to end of last range
                minor_ = major_->second.end();
            }

            if (minor_ == major_->second.begin()) {
                --major_;        // back off to end of previous range
                minor_ = major_->second.end();
            }
            --minor_;
            --index_;
        }

        difference_type distance_to(disjoint_cursor const& other) const
        {
            return other.index_ - this->index_;
        }
        
        reference dereference() const
        {
            return *minor_;
        }

        void advance(difference_type n)
        {
            if (n == 0) return;

            // first, local distance from minor_ to current range begin
            difference_type ldist = major_->first - index_; // negative

            // We are too high, back up.
            while (n < ldist) {
                const size_t old_index = index_;
                --major_;
                index_ = major_->first;
                minor_ = major_->second.begin();
                n += old_index - index_;
                ldist = 0;
            }

            // ldist is either still negative or zero.  next find
            // distance from where we are to the end of the current
            // range.
            ldist = major_->second.size() + ldist;

            // We are too low, go forward.
            while (n >= ldist) {
                const size_t old_index = index_;
                ++major_;
                index_ = major_->first;
                minor_ = major_->second.begin();
                ldist = major_->second.size();
                n -= index_ - old_index;
            }

            // just right
            index_ += n;
            minor_ += n;
        }
    };
}

#endif
