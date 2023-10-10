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
#include <memory>

#include <boost/iterator/iterator_facade.hpp>


namespace WireCell :: NaryTree {

    
    /** Iterator and range on children in a node */
    template<typename Value> class child_iter;

    /** Iterator and range depth first, parent then children */
    template<typename Value> class depth_ptc_iter;

    /** Iterator and range depth first, children then parent */
    template<typename Value> class depth_ctp_iter;


    /** A tree node. */
    template<class Value>
    class Node {

      public:

        using value_type = Value;
        using self_type = Node<Value>;
        using owned_ptr = std::unique_ptr<self_type>;

        Node() = default;
        ~Node() = default;

        // Copy value.
        Node(const value_type& val) : value(val) {}

        // Move value.
        Node(value_type&& val) : value(val) {}

        // Insert a child by its value copy.
        void insert(const value_type& val) {
            auto child = std::make_shared<self_type>(val);
            child->parent = this;
            children_.emplace_back(child);
        }

        // Insert a child by its value move.
        void insert(value_type&& val) {
            auto child = std::make_shared<self_type>(val);
            child->parent = this;
            children_.emplace_back(child);
        }

        // Insert a child by node pointer.
        void insert(owned_ptr node) {
            node->parent = this;
            children_.push_back(std::move(node));
        }
        
        // Access the value
        value_type value;

        // Access parent that holds this as a child.
        self_type* parent{nullptr};

        // Range-based access to children
        child_iter<Value> children() { return child_iter<Value>(this); }
        const child_iter<Value> children() const { return child_iter<Value>(this); }

        // 

      private:
        friend class child_iter<Value>;
        std::vector<owned_ptr> children_;

    };

    // Iterator and range for descent, parents then children
    template<typename Value>
    class depth_ptc_iter : public boost::iterator_facade<
        depth_ptc_iter<Value>
        , Value
        , boost::forward_traversal_tag>
    {
      public: 
        using value_type = Value;
        using node_type = Node<Value>;

        depth_ptc_iter() = default;

        explicit depth_ptc_iter(node_type* node) : stack{ {node,0} } {}

        template<typename O>
        depth_ptc_iter(depth_ptc_iter<O> const& o) : stack(o.stack.begin(), o.stack.end()) {}

        // Range interface
        depth_ptc_iter<Value> begin() { return depth_ptc_iter<Value>(node_); }
        depth_ptc_iter<Value> end() { return depth_ptc_iter<Value>(); }

      private:
        friend class boost::iterator_core_access;

        void increment()
        {
            if (stack.empty()) {
                throw std::out
            }

            if (!node) return;
            ++index;
            if (index < node->children_.size()) {
                return;
            }
            node = nullptr;     // end            
        }
        bool equal(depth_ptc_iter const& other) const {
            if (node != other.node) {
                return false;
            }
            return !node or index == other.index;
        }
        Value& dereference() const {
            return node->children_[index]->value;
        }

        using address_type = std::pair<node_type*, size_t>;
        std::vector<address_type> stack;

    }

    // Iterator and range on node children.
    template<typename Value>
    class child_iter : public boost::iterator_facade<
        child_iter<Value>
        , Value
        , boost::forward_traversal_tag>
    {
      public:
        using value_type = Value;
        using node_type = Node<Value>;

        child_iter() = default;
        explicit child_iter(node_type* node) : node(node), index(0) {}
        template<typename O>
        child_iter(child_iter<O> const& o) : node(o.node), index(o.index) {}

        // Range interface
        child_iter<Value> begin() { return child_iter<Value>(node_); }
        child_iter<Value> end() { return child_iter<Value>(); }

      private:
        friend class boost::iterator_core_access;
        void increment() {
            if (!node) return;
            ++index;
            if (index < node->children_.size()) {
                return;
            }
            node = nullptr;     // end            
        }
        bool equal(child_iter const& other) const {
            if (node != other.node) {
                return false;
            }
            return !node or index == other.index;
        }
        Value& dereference() const {
            return node->children_[index]->value;
        }
        node_type* node{nullptr};
        size_t index{0};
    };



}
