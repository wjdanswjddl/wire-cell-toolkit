#ifndef WIRECELLUTIL_POINTCLOUDCOORDINATES
#define WIRECELLUTIL_POINTCLOUDCOORDINATES

#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/Logging.h"

#include <boost/iterator/iterator_adaptor.hpp>
#include <memory>

namespace WireCell::PointCloud {

    /**
       A coordinate point is a vector-like slice across the arrays of
       a selection of a dataset at a given index along the major axis.

       coordinate_point cp(selection_of_four,42);
       x = cp[0];
       y = cp[1];
       z = cp[2];
       t = cp[3];

       This is primarily intended to provide a "value type" to the
       coordinate_iterator though it may also be used directly.

       Note, a pointer to the selection is retained.  The caller must
       assure the lifetime of the selection exceeds that of the
       coordinate_point.

    */
    template<typename ElementType>
    class coordinate_point {
      public:
        using value_type = ElementType;

        using selection_t = Dataset::selection_t;

        coordinate_point(selection_t* selptr=nullptr, size_t ind=0)
            : selptr(selptr), index_(ind) {}

        coordinate_point(selection_t& sel, size_t ind=0)
            : selptr(&sel), index_(ind) {}

        coordinate_point(const coordinate_point& o)
            : selptr(o.selptr), index_(o.index_) {}

        coordinate_point& operator=(const coordinate_point& o)
        {
            selptr = o.selptr;
            index_ = o.index_;
            return *this;
        }

        // number of dimensions of the point.
        size_t size() const {
            if (selptr) {
                return selptr->size();
            }
            return 0;
        }

        // no bounds checking
        value_type operator[](size_t dim) const {
            auto arr = (*selptr)[dim];
            return arr->element<ElementType>(index_);
        }

        void assure_valid(size_t dim) const {
            if (!selptr) {
                throw std::out_of_range("coordinate point has no selection");
            }
            const size_t ndims = selptr->size();
            if (!ndims) {
                throw std::out_of_range("coordinate point has empty selection");
            }   
            if (dim >= ndims) {
                throw std::out_of_range("coordinate point dimension out of range");
            }
            if (index_ >= (*selptr)[0]->size_major()) {
                throw std::out_of_range("coordinate point index out of range");
            }
        }

        // size_t npoints() const
        // {
        //     if (!selptr || selptr->empty() || (*selptr)[0].empty()) return 0;
        //     return (*selptr)[0]->size_major();
        // }

        value_type at(size_t dim) const {
            assure_valid(dim);
            return (*this)[dim];
        }

        size_t& index() { return index_; }
        size_t index() const { return index_; }

        bool operator==(const coordinate_point& other) const {
            return index_ == other.index_ && selptr == other.selptr;
        }

      private:
        selection_t* selptr;
        size_t index_;
    };


    /**
       An iterator over a coordinate range.
     */
    template<typename PointType>
    class coordinate_iterator
        : public boost::iterator_facade<coordinate_iterator <PointType>
                                        , PointType
                                        , boost::random_access_traversal_tag>
    {
      public:
        using self_type = coordinate_iterator<PointType>;
        using base_type = boost::iterator_facade<self_type
                                                 , PointType
                                                 , boost::random_access_traversal_tag>;
        using difference_type = typename base_type::difference_type;
        using value_type = typename base_type::value_type;
        using pointer = typename base_type::pointer;
        using reference = typename base_type::reference;

        using selection_t = typename PointType::selection_t;

        coordinate_iterator(selection_t* sel=nullptr, size_t ind=0)
            : point(sel, ind)
        {
        }

        coordinate_iterator(const coordinate_iterator& o)
            : point(o.point)
        {            
        }

        template<typename OtherIter>
        coordinate_iterator(OtherIter o)
            : point(*o)
        {
        }

        coordinate_iterator& operator=(const coordinate_iterator& o)
        {
            point = o.point;
            return *this;
        }

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
            size_t index = point.index();
            point.index() = index + n;
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

     */
    template<typename PointType>
    class coordinate_range {
      public:
        using iterator = coordinate_iterator<PointType>;
        using const_iterator = coordinate_iterator<PointType const>;

        using selection_t = typename PointType::selection_t;

        coordinate_range() = delete;
        ~coordinate_range() {
        }

        // The selection to transpose must remain in place.
        //
        explicit coordinate_range(selection_t& sel)
            : selptr(&sel)
        {
        }
        explicit coordinate_range(selection_t* selptr)
            : selptr(selptr)
        {
        }

        /// Number of coordinate points.
        size_t size() const {
            if (!selptr || selptr->empty()) return 0;
            return (*selptr)[0]->size_major();
        }

        iterator begin()  { 
            return iterator(selptr, 0);
        }
        iterator end()  {
            return iterator(selptr, size());
        }
        const_iterator begin() const { 
            return const_iterator(selptr, 0);
        }
        const_iterator end() const {
            return const_iterator(selptr, size());
        }

      private:
        selection_t* selptr;
        
    };


}

#endif


