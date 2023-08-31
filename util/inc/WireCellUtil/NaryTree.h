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

namespace WireCell::NaryTree {

    
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
        // node owns child nodes
        using owned_ptr = std::unique_ptr<self_type>;
        // holds the children
        using nursery_type = std::list<owned_ptr>; 
        using sibling_iterator = typename nursery_type::iterator;

        Node() = default;
        ~Node() = default;

        // Copy value.
        Node(const value_type& val) : value(val) {}

        // Move value.
        Node(value_type&& val) : value(std::move(val)) {}

        // Insert a child by its value copy.
        void insert(const value_type& val) {
            insert(std::make_unique<self_type>(val));
        }

        // Insert a child by its value move.
        void insert(value_type&& val) {
            owned_ptr p = std::make_unique<self_type>(std::move(val));
            insert(std::move(p));
        }

        // Insert a child by node pointer.
        void insert(owned_ptr node) {
            node->parent = this;
            children_.push_back(std::move(node));
            children_.back()->sibling = std::prev(children_.end());
        }
        
        // Access the value
        value_type value;

        // Access parent that holds this as a child.
        self_type* parent{nullptr};

        // Iterator locating self in list of siblings.
        sibling_iterator sibling;

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

        // todo:
        // remove child


      private:

        nursery_type children_;

    };


    // Iterator and range for descent.  First parent then children (ptc).
    template<typename Value>
    class depth_ptc_iter : public boost::iterator_facade<
        depth_ptc_iter<Value>
        , Value
        , boost::forward_traversal_tag>
    {
      public: 
        using value_type = Value;
        using node_type = Node<Value>;

        // Current node.  If nullptr, iterator is at "end".
        node_type* node{nullptr};

        // default is nodeless and indicates "end"
        depth_ptc_iter() : node(nullptr) {}

        explicit depth_ptc_iter(node_type* node) : node(node) { }

        // template<typename O>
        // depth_ptc_iter(depth_ptc_iter<O> const& o) : stack(o.stack.begin(), o.stack.end()) {}

        // Range interface
        depth_ptc_iter<Value> begin() { return depth_ptc_iter<Value>(node); }
        depth_ptc_iter<Value> end() { return depth_ptc_iter<Value>(); }

      private:
        friend class boost::iterator_core_access;

        // If we have child, we go there.
        // If no child, we go to next sibling.
        // If no next sibling we go toward root taking first ancestor sibling found.
        // If still nothing, we are at the end.
        void increment()
        {
            std::cerr << "increment: start\n";

            if (!node) return; // already past the end, throw here?

            std::cerr << "increment: have node " << node->value << "\n";

            {                   // if we have child, go there
                auto* first = node->first();
                if (first) {
                    node = first;
                    std::cerr << "increment: go child " << node->value << "\n";
                    return;
                }
            }

            std::cerr << "increment: no children " << node->value << "\n";

            while (true) {

                auto* sib = node->next();
                if (sib) {
                    node = sib;
                    std::cerr << "increment: go sibling " << node->value << "\n";
                    return;
                }
                if (node->parent) {
                    node = node->parent;
                    std::cerr << "increment: try parent " << node->value << "\n";
                    continue;
                }
                std::cerr << "increment: no parent, no sibs " << node->value << "\n";
                break;
            }
            std::cerr << "increment: at end\n";
            node = nullptr;     // end
        }

        bool equal(depth_ptc_iter const& other) const {
            return node == other.node;
        }
        Value& dereference() const {
            return node->value;
        }
    };

}

#endif
