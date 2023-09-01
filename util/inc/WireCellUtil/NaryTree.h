/** N-ary tree.
   
    A tree is a graph where the vertices (nodes) are organized in
    parent/child relationship via graph edges.  A node may connect to
    at most one parent.  If a node has no parent it is called the
    "root" node.  A node may edges to zero or more children nodes for
    which the node is their parent.  Edges are bi-directional with the
    "downward" direction being identified as going from a parent to a
    child.

    In this implementation there is no explicit tree object.  The
    singular root node can be considered conceptually as "the tree"
    though it is possible to naviate to all nodes given any node in
    "the" tree.

    The tree is built by inserting child values or nodes into a
    parent node.

    Several types of iterators and ranges provide different methods to
    descend through the tree to provide access to the node value.

*/

#ifndef WIRECELLUTIL_NARYTREE
#define WIRECELLUTIL_NARYTREE

#include "WireCellUtil/Exceptions.h"

#include <vector>
#include <list>
#include <memory>

#include <iostream>             // debug

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/indirect_iterator.hpp>
// Telling the truth
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

namespace WireCell::NaryTree {

    template<typename Iter>
    struct iter_range {
        Iter b,e;
        Iter begin() { return b; }
        Iter end()   { return e; }
    };

    /** Iterator and range on children values */
    template<typename Value> class child_value_iter;
    template<typename Value> class child_value_const_iter;
    
    /** Iterator and range depth first, parent then children */
    template<typename Value> class depth_iter;
    template<typename Value> class depth_const_iter;

    /** A tree node. */
    template<class Value>
    class Node {

      public:

        using value_type = Value;
        using self_type = Node<Value>;
        // node owns child nodes
        using owned_ptr = std::unique_ptr<self_type>;
        // holds the children
        using nursery_type = std::list<owned_ptr>; 
        using sibling_iter = typename nursery_type::iterator;
        using sibling_const_iter = typename nursery_type::const_iterator;

        // using depth_range = depth_iter<Value>;

        Node() = default;
        ~Node() = default;

        // Copy value.
        Node(const value_type& val) : value(val) {}

        // Move value.
        Node(value_type&& val) : value(std::move(val)) {}

        // Insert a child by its value copy.
        Node* insert(const value_type& val) {
            return insert(std::make_unique<self_type>(val));
        }

        // Insert a child by its value move.
        Node* insert(value_type&& val) {
            owned_ptr p = std::make_unique<self_type>(std::move(val));
            return insert(std::move(p));
        }

        // Insert a child by node pointer.
        Node* insert(owned_ptr node) {
            node->parent = this;
            children_.push_back(std::move(node));
            children_.back()->sibling = std::prev(children_.end());
            return children_.back().get();
        }

        // Return iterator to node or end.  This is a linear search.
        sibling_iter find(const Node* node) {
            return std::find_if(children_.begin(), children_.end(),
                                [&](const owned_ptr& up) {
                                    return up.get() == node;
                                });
        }
        sibling_const_iter find(const Node* node) const {
            return std::find_if(children_.cbegin(), children_.cend(),
                                [&](const owned_ptr& up) {
                                    return up.get() == node;
                                });
        }
        
        // Remove and return child node.
        owned_ptr remove(sibling_iter sib) {
            if (sib == children_.end()) return nullptr;
            owned_ptr ret = std::move(*sib);
            children_.erase(sib);
            return ret;
        }

        // Remove child node.  Searches children.  Return child node
        // in owned pointer if found else nullptr.
        owned_ptr remove(Node* node) {
            auto it = find(node);
            return remove(it);
        }

        // Access the value
        value_type value;

        // Access parent that holds this as a child.
        self_type* parent{nullptr};

        // Iterator locating self in list of siblings.
        sibling_iter sibling;

        self_type* first() const {
            if (children_.empty()) return nullptr;
            return children_.front().get();
        }

        self_type* last() const {
            if (children_.empty()) return nullptr;
            return children_.back().get();
        }

        // Return left/previous/older sibling, nullptr if we are first.
        self_type* prev() const {
            if (!parent) return nullptr;
            const auto& sibs = parent->children_;
            if (sibs.empty()) return nullptr;

            if (sibling == sibs.begin()) {
                return nullptr;
            }
            auto sib = sibling;
            --sib;
            return sib->get();
        }
        // Return right/next/newerr sibling, nullptr if we are last.
        self_type* next() const {
            if (!parent) return nullptr;
            const auto& sibs = parent->children_;
            if (sibs.empty()) return nullptr;

            auto sib = sibling;
            ++sib;
            if (sib == sibs.end()) {
                return nullptr;
            }
            return sib->get();
        }

        // Access list of child nodes.
        const nursery_type& children() const { return children_; }
        nursery_type& children() { return children_; }

        using child_value_range = iter_range<child_value_iter<Value>>;
        auto child_values() {
            return child_value_range{
                child_value_iter<Value>(children_.begin()),
                child_value_iter<Value>(children_.end()) };
        }
        using child_value_const_range = iter_range<child_value_const_iter<Value>>;
        auto child_values() const {
            return child_value_const_range{
                child_value_const_iter<Value>(children_.cbegin()),
                child_value_const_iter<Value>(children_.cend()) };
        }

        using child_node_iter = boost::indirect_iterator<sibling_iter>;
        using child_node_range = iter_range<child_node_iter>;
        auto child_nodes() {
            return child_node_range{ children_.begin(), children_.end() };
        }
        using child_node_const_iter = boost::indirect_iterator<sibling_const_iter>;
        using child_node_const_range = iter_range<child_node_const_iter>;
        auto child_nodes() const {
            return child_node_const_range{ children_.cbegin(), children_.cend() };
        }
                
        // Iterable range for depth first traversal, parent then children.
        depth_iter<Value> depth()  { return depth_iter<Value>(this); }
        depth_const_iter<Value> depth() const { return depth_const_iter<Value>(this); }

      private:

        // friend class child_value_iter<Value>;
        nursery_type children_;

    };                          // Node



    //
    // Iterator on node childrens' value.
    //
    template<typename Value>
    class child_value_iter : public boost::iterator_adaptor<
        child_value_iter<Value>              // Derived
        , typename Node<Value>::sibling_iter // Base
        , Value                              // Value
        , boost::bidirectional_traversal_tag>
    {
      private:
        struct enabler {};  // a private type avoids misuse

      public:

        using sibling_iter = typename Node<Value>::sibling_iter;

        child_value_iter()
            : child_value_iter::iterator_adaptor_()
        {}

        explicit child_value_iter(sibling_iter sit)
            : child_value_iter::iterator_adaptor_(sit)
        {}

        template<typename OtherValue>
        child_value_iter(child_value_iter<OtherValue> const& other
                   , typename boost::enable_if<
                   boost::is_convertible<OtherValue*,Value*>
                   , enabler
                   >::type = enabler()
            )
            : child_value_iter::iterator_adaptor_(other->children().begin())
        {}

      private:
        friend class boost::iterator_core_access;
        Value& dereference() const {
            auto sib = this->base();
            return (*sib)->value;
        }
    };    

    // Const iterator on node childrens' value.
    template<typename Value>
    class child_value_const_iter : public boost::iterator_adaptor<
        child_value_const_iter<Value>              // Derived
        , typename Node<Value>::sibling_const_iter // Base
        , Value                                    // Value
        , boost::bidirectional_traversal_tag>
    {
      private:
        struct enabler {};  // a private type avoids misuse

      public:

        using sibling_const_iter = typename Node<Value>::sibling_const_iter;

        child_value_const_iter()
            : child_value_const_iter::iterator_adaptor_()
        {}

        explicit child_value_const_iter(sibling_const_iter sit)
            : child_value_const_iter::iterator_adaptor_(sit)
        {}

        // convert non-const
        template<typename OtherValue>
        child_value_const_iter(child_value_iter<OtherValue> const& other
                   , typename boost::enable_if<
                   boost::is_convertible<OtherValue*,Value*>
                   , enabler
                   >::type = enabler()
            )
            : child_value_const_iter::iterator_adaptor_(other->children().begin())
        {}

      private:
        friend class boost::iterator_core_access;

        // All that just to provide:
        Value& dereference() const {
            auto sib = this->base();
            return (*sib)->value;
        }
    };    



    // Iterator and range for descent.  First parent then children.
    template<typename Value>
    class depth_iter : public boost::iterator_facade<
        depth_iter<Value>
        , Value
        , boost::forward_traversal_tag>
    {
      private:
        struct enabler {};
      public: 

        using node_type = Node<Value>;

        // Current node.  If nullptr, iterator is at "end".
        node_type* node{nullptr};

        // default is nodeless and indicates "end"
        depth_iter() : node(nullptr) {}

        explicit depth_iter(node_type* node) : node(node) { }

        template<typename OtherValue>
        depth_iter(depth_iter<OtherValue> const & other
                   // , typename boost::enable_if<
                   // boost::is_convertible<OtherValue*,Value*>
                   // , enabler
                   // >::type = enabler()
            ) : node(other.node) {}

        // Range interface
        depth_iter<Value> begin() { return depth_iter<Value>(node); }
        depth_iter<Value> end() { return depth_iter<Value>(); }

      private:
        friend class boost::iterator_core_access;

        // If we have child, we go there.
        // If no child, we go to next sibling.
        // If no next sibling we go toward root taking first ancestor sibling found.
        // If still nothing, we are at the end.
        void increment()
        {
            if (!node) return; // already past the end, throw here?
            {                   // if we have child, go there
                auto* first = node->first();
                if (first) {
                    node = first;
                    return;
                }
            }

            while (true) {

                auto* sib = node->next();
                if (sib) {
                    node = sib;
                    return;
                }
                if (node->parent) {
                    node = node->parent;
                    continue;
                }
                break;
            }
            node = nullptr;     // end
        }

        template <typename OtherValue> 
        bool equal(depth_iter<OtherValue> const& other) const {
            return node == other.node;
        }

        Value& dereference() const {
            return node->value;
        }
    };

    // Iterator and range for descent.  First parent then children.
    template<typename Value>
    class depth_const_iter : public boost::iterator_facade<
        depth_const_iter<Value>
        , Value const
        , boost::forward_traversal_tag>
    {
      private:
        struct enabler {};
      public: 

        using node_type = Node<Value> const;

        // Current node.  If nullptr, iterator is at "end".
        node_type* node{nullptr};

        // default is nodeless and indicates "end"
        depth_const_iter() : node(nullptr) {}

        explicit depth_const_iter(node_type* node) : node(node) { }

        template<typename OtherValue>
        depth_const_iter(depth_iter<OtherValue> const & other) : node(other.node) {}
        template<typename OtherValue>
        depth_const_iter(depth_const_iter<OtherValue> const & other) : node(other.node) {}

        // Range interface
        depth_const_iter<Value> begin() { return depth_const_iter<Value>(node); }
        depth_const_iter<Value> end() { return depth_const_iter<Value>(); }

      private:
        friend class boost::iterator_core_access;

        // If we have child, we go there.
        // If no child, we go to next sibling.
        // If no next sibling we go toward root taking first ancestor sibling found.
        // If still nothing, we are at the end.
        void increment()
        {
            if (!node) return; // already past the end, throw here?

            {                   // if we have child, go there
                auto* first = node->first();
                if (first) {
                    node = first;
                    return;
                }
            }

            while (true) {

                auto* sib = node->next();
                if (sib) {
                    node = sib;
                    return;
                }
                if (node->parent) {
                    node = node->parent;
                    continue;
                }
                break;
            }
            node = nullptr;     // end
        }

        template <typename OtherValue> 
        bool equal(depth_iter<OtherValue> const& other) const {
            return node == other.node;
        }
        template <typename OtherValue> 
        bool equal(depth_const_iter<OtherValue> const& other) const {
            return node == other.node;
        }

        const Value& dereference() const {
            return node->value;
        }
    };

}

#endif
