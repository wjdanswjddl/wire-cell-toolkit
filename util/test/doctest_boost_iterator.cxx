#include "WireCellUtil/Logging.h"
#include "WireCellUtil/Type.h"
#include "WireCellUtil/doctest.h"

#include <boost/iterator/iterator_adaptor.hpp>

using namespace WireCell;
using spdlog::debug;

template<typename ValueType>
class my_iterator
    : public boost::iterator_facade<my_iterator<ValueType>
                                        , ValueType
                                        , boost::random_access_traversal_tag>
{
  public:
    using self_type = my_iterator<ValueType>;
    using base_type = boost::iterator_facade<self_type
                                             , ValueType
                                             , boost::random_access_traversal_tag>;
    using difference_type = typename base_type::difference_type;
    using value_type = typename base_type::value_type;
    using pointer = typename base_type::pointer;
    using reference = typename base_type::reference;
    
    my_iterator() : value(0), vptr(&value)
    {
        debug("ValueType: {}", type<ValueType>());
        debug("value_type: {}", type<value_type>());
        debug("pointer: {}", type<pointer>());
        debug("reference: {}", type<reference>());
        debug("self_type: {}", type<self_type>());
    }

    my_iterator(const my_iterator&) = default;
    my_iterator(my_iterator&&) = default;
    my_iterator& operator=(const my_iterator&) = default;
    my_iterator& operator=(my_iterator&&) = default;
    
    explicit my_iterator(const ValueType& val)
        : value(val) {}

  private:
    value_type value;
    value_type *vptr;
    friend class boost::iterator_core_access;

    bool equal(const my_iterator& o) const {
        return value == o.value;
    }

    void increment () {
        ++value;
    }
    void decrement () {
        --value;
    }
    void advance (difference_type n) {
        value += n;
    }

    difference_type
    distance_to(self_type const& other) const
    {
        return other.value - this->value;
    }

    reference dereference() const
    {
        debug("value_type: {} in dereference", type<value_type>());
        debug("value: {} in dereference", type(value));
        debug("&value: {} in dereference", type(&value));
        debug("vptr: {} in dereference", type(vptr));
        debug("*vptr: {} in dereference", type(*vptr));

        // return *vptr;
        // return value;
        value_type* tmp = vptr;
        // value_type* tmp = &value;
        return *tmp;
    }
};

TEST_CASE("boost iterator facade value")
{
    my_iterator<int> myit;
    
    CHECK (0 == *myit);
    ++myit;
    CHECK (1 == *myit);
}
