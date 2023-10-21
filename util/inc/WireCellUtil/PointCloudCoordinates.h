#ifndef WIRECELLUTIL_POINTCLOUDCOORDINATES
#define WIRECELLUTIL_POINTCLOUDCOORDINATES

#include "WireCellUtil/PointCloudArray.h"

#include <boost/iterator/iterator_adaptor.hpp>
#include <memory>

namespace WireCell::PointCloud {

    /**
       A coordinate point is a vector-like slice across the arrays of
       a coordinate selection at a given index.

       coordinate_point cp(selection_of_four,42);
       x = cp[0];
       y = cp[1];
       z = cp[2];
       t = cp[3];

       Note, the coordinate_point retains a pointer to the selection.
       Caller must assure the lifetime of the selection exceeds that
       of the coordinate_point.

    */
    template<typename ElementType, typename SelectionType=selection_t>
    class coordinate_point {
      public:
        using value_type = ElementType;

        coordinate_point()
            : selection_(nullptr), index_(0) {}

        explicit coordinate_point(SelectionType& sel, size_t ind=0)
            : selection_(&sel), index_(ind) {}

        explicit coordinate_point(SelectionType* sel, size_t ind=0)
            : selection_(sel), index_(ind) {}

        size_t size() const {
            return selection_->size();
        }

        value_type operator[](size_t dim) const {
            // selections hold ref to const arrays 
            const Array& arr = (*selection_)[dim];
            return arr.element<ElementType>(index_);
        }

        void assure_valid(size_t dim) const {
            if (dim >= size()) {
                throw std::out_of_range("coordinate dimension out of range");
            }
            if (index_ >= selection_->at(0).get().size_major()) {
                throw std::out_of_range("coordinate index out of range");
            }
        }

        value_type at(size_t dim) const {
            assure_valid(dim);
            return (*this)[dim];
        }

        size_t& index() { return index_; }
        size_t index() const { return index_; }

        bool operator==(const coordinate_point& other) const {
            return selection_ == other.selection_ && index_ == other.index_;
        }

      private:
        SelectionType* selection_;
        size_t index_;
    };


    /**
       Iterator over a coordinate range.
     */
    template<typename PointType, typename SelectionType=selection_t>
    class coordinate_iterator
        : public boost::iterator_facade<coordinate_iterator <PointType, SelectionType>
                                        , PointType
                                        , boost::random_access_traversal_tag>
    {
      public:
        using self_type = coordinate_iterator<PointType, SelectionType>;
        using base_type = boost::iterator_facade<self_type
                                                 , PointType
                                                 , boost::random_access_traversal_tag>;
        using difference_type = typename base_type::difference_type;
        using value_type = typename base_type::value_type;
        using pointer = typename base_type::pointer;
        using reference = typename base_type::reference;

        coordinate_iterator()
            : point() {}

        coordinate_iterator(const coordinate_iterator&) = default;
        coordinate_iterator(coordinate_iterator&&) = default;
        coordinate_iterator& operator=(const coordinate_iterator&) = default;
        coordinate_iterator& operator=(coordinate_iterator&&) = default;

        coordinate_iterator(SelectionType& sel, size_t ind)
            : point(&sel,ind) {}

        coordinate_iterator(SelectionType* sel, size_t ind)
            : point(sel,ind) {}

      private:
        mutable value_type point;

      private:
        friend class boost::iterator_core_access;

        bool equal(const coordinate_iterator& o) const {
            return point == o.point;
        }

        void increment () {
            ++point.index();
        }
        void decrement () {
            --point.index();
        }
        void advance (difference_type n) {
            point.index() += n;
        }

        difference_type
        distance_to(self_type const& other) const
        {
            return other.point.index() - this->point.index();
        }

        reference dereference() const
        {
            return point;
        }
    };


    /**
       A transpose of a point cloud selection.

       coordinate_range<double> cr(selection);
       for (const coordinate_point& cp : cr) {
           assert(cp.size() == selection.size());
           auto dim0 = cp[0];
           for (auto x : c) {
               //...
           }
       }

     */
    template<typename PointType, typename SelectionType=selection_t>
    class coordinate_range {
      public:
        using iterator = coordinate_iterator<PointType, SelectionType>;
        using const_iterator = coordinate_iterator<PointType const, SelectionType const>;

        coordinate_range() : selection_(nullptr) {}

        explicit coordinate_range(SelectionType& sel)
            : selection_(&sel) {}

        size_t size() const {
            if (!selection_) return 0;
            if (selection_->empty()) return 0;
            const Array& arr = selection_->at(0);
            return arr.size_major();
        }

        iterator begin()  { 
            return iterator(selection_, 0);
        }
        iterator end()  {
            return iterator(selection_, size());
        }
        const_iterator begin() const { 
            return const_iterator(selection_, 0);
        }
        const_iterator end() const {
            return const_iterator(selection_, size());
        }

      private:
        SelectionType* selection_;
        
    };


}

#endif


