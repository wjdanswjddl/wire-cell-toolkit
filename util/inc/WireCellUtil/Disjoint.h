#ifndef WIRECELLUTIL_DISJOINT
#define WIRECELLUTIL_DISJOINT

#include <boost/iterator/iterator_adaptor.hpp>
#include <stdexcept>

namespace WireCell {

    // Iterate on a outer range of inner ranges.
    template <typename MajorIter,
              typename MinorIter = typename MajorIter::iterator::value_type::iterator,
              typename MinorValue = typename MinorIter::value_type>
    class disjoint_iterator
        : public boost::iterator_facade<disjoint_iterator<MajorIter, MinorIter, MinorValue>
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
            boost::iterator_facade<disjoint_iterator<MajorIter, MinorIter, MinorValue>
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
        disjoint_iterator(major_iterator b, major_iterator e, size_t index=0)
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

        bool equal(disjoint_iterator const& o) const {
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

        difference_type distance_to(disjoint_iterator const& other) const
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
