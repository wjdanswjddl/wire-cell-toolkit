#ifndef WIRECELLAUX_CLUSTERARRAYS
#define WIRECELLAUX_CLUSTERARRAYS

#include "ClusterHelpers.h"
#include "WireCellIface/ICluster.h"
#include <boost/multi_array.hpp>
#include <boost/range/counting_range.hpp>

namespace WireCell::Aux {


    /**
       A small helper to it just a bit easier to loop over a node type
       partition inside a node array using a node type array.
    
       ClusterArray ca = ...;
       for (auto ind : index_range(ca.node_ranges()[4])) {
           auto d = data_array[ind];
       }

       See also ca.ntype_range(char code).
    */
    template<typename ArrayType>
    boost::iterator_range<boost::counting_iterator<size_t>>
    index_range(const ArrayType& arr) {
        return boost::counting_range(arr[0], arr[0] + arr[1]);
    }

    /**
       ClusterArrays produces array representations from ICluster data.

       See also AnodeArrays for array representations all anode wires.

       See also the aux/docs/ClusterArrays.org document.
    */
    class ClusterArrays {
      public:

        ClusterArrays(const cluster_graph_t& graph);

        // Clear all stored data.  This will invalidate any arrays
        // previously accessed and held by const reference.
        void clear();

      public:
        // Type codes of all nodes.
        const std::string& node_codes() const;
      private:
        mutable std::string m_node_codes;

      public:
        // A vertext descriptor array (specially indexed by graph
        // vertex descriptors) holding node array indices.  This array
        // is only required if node arrays are accessed in graph node
        // order.
        using vdesc_indices_t = boost::multi_array<size_t, 1>;
        const vdesc_indices_t& vdesc_indices() const;
      private:
        mutable vdesc_indices_t m_vdesc_indices;

      public:
        // A node array giving graph vertex descriptors.  This array
        // is only required if graph nodes are to be accessed in
        // node-array order.
        using vdescs_t = boost::multi_array<cluster_vertex_t, 1>;
        const vdescs_t& graph_vertices() const;
      private:
        mutable vdescs_t m_vdescs;
        
      public:
        // Nedge rows, columns are indices (not vdescs!) for
        // (tail,head) nodes.  Ordered by tail then head.
        using edges_t = boost::multi_array<size_t, 2>;
        const edges_t& edges() const;
      private:
        mutable edges_t m_edges;


      public:
        // Helper to get one range by type code.  Eg, iterate over
        // slices:
        //
        // for (auto ind : ca.ntype_range('s') {
        //    auto d = all_node_data_array[ind];
        // }
        boost::iterator_range<boost::counting_iterator<size_t>>
        ntype_range(char code) const;
        
        /// Return (5,2) array of all node parition ranges.
        using node_ranges_t = boost::multi_array<size_t, 2>;
        const node_ranges_t& node_ranges() const;
      private:
        mutable node_ranges_t m_node_ranges;

      public:
        // The full node array of node idents.  Note, m-node idents
        // are taken as the ident of their first channel.
        using idents_t = boost::multi_array<int, 1>;
        const idents_t& idents() const;
      private:
        mutable idents_t m_idents;

      public:
        // The full node array (if code = '0' or otherwise a node
        // code) or a node type array (if code in "cwbsm") of signals.
        // First column is central value, second is uncertainty.  A
        // w-node has zero signal value and uncertainty.
        using value_t = ISlice::value_t;
        using signals_t = boost::multi_array<float, 2LU>;
        const signals_t& signals() const;
      private:
        mutable signals_t m_signals;


      public:
        // Slice node type array of shape (Nslices, 2) giving slice
        // duration with columns (start time, time span).
        using slice_durations_t = boost::multi_array<float, 2>;
        const slice_durations_t& slice_durations() const;
      private:
        mutable slice_durations_t m_slice_durations;

      public:
        // Wire node type array giving wire endpoints. Shape is
        // (Nwire, 2, 3) where second dimension is first tail and
        // second head wire endpoint and last dimension is 3-vector in
        // global detector volume coordinates.
        using wire_endpoints_t = boost::multi_array<float, 3>;
        const wire_endpoints_t& wire_endpoints() const;
      private:
        mutable wire_endpoints_t m_wire_endpoints;

      public:
        // Wire addresses is (Nwire, 4) with columns (WIP, segment, channelID, planeID).
        using wire_addresses_t = boost::multi_array<int, 2>;
        const wire_addresses_t& wire_addresses() const;
      private:
        mutable wire_addresses_t m_wire_addresses;

      public:

        // Blob node type array of shape (Nblobs, 3, 2).  Each (3,2)
        // "row" gives the bounds of the blob in terms of the WIP
        // indices for a pair of wires in each of 3 wire planes.
        // Important note, this provides only partial bounds as blobs
        // are also bound in the horizontal and vertical direction by
        // the active area of the anode.  These bounds are not
        // provided here.  See AnodeArrays for ways access them.
        using blob_shapes_t = boost::multi_array<int, 3>;
        const blob_shapes_t& blob_shapes() const;
      private:
        mutable blob_shapes_t m_blob_shapes;
        

      private:
        // fill node_ranges, vdescs and vdesc_indices arrays
        void core_nodes() const;
        // filel wire node type arrays
        void core_wires() const;

        const cluster_graph_t& m_graph;
    };

}

#endif
