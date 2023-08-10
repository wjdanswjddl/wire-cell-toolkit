#ifndef WIRECELLSIO_CLUSTERFILESINK
#define WIRECELLSIO_CLUSTERFILESINK

#include "WireCellUtil/Stream.h"
#include "WireCellIface/IClusterSink.h"
#include "WireCellIface/ITerminal.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"


#include <boost/iostreams/filtering_stream.hpp>

#include <string>

namespace WireCell::Sio {

    /** Sink cluster related information to file.
     *
     * This component will save ICluster and referenced objects.
     */
    class ClusterFileSink : public Aux::Logger,
                            public IClusterSink, public ITerminal,
                            public IConfigurable
    {
    public:
        ClusterFileSink();
        virtual ~ClusterFileSink();

        virtual void finalize();

        virtual void configure(const WireCell::Configuration& cfg);
        virtual WireCell::Configuration default_configuration() const;
        
        virtual bool operator()(const ICluster::pointer& cluster);

    private:
        /**
           Configuration:
           
           outname = "cluster-file.tar.gz"

           Name the output stream.  The stream format will be guessed
           based on the name.  

           Currently, a tar stream with optional compression is
           supported by providing a file name.  Compression is as
           supported by custard and guessed based on file name
           extension.  Examples include .tar, .tar.gz, .tgz, .tar.xz,
           .txz, .tar.pixz, .tpxz, .tix.  (Last three require pixz
           installed).
        */
        std::string m_outname{"cluster-file.tar.gz"};

        /**
           format = "json"

           The output stream contains one or more "files" in a given
           format.  Supported formats:

           - json :: clusters are written to JSON files in
             "cluster graph schema" form.

           - numpy :: clusters are written to Numpy files in the
             stream in "cluter array schema" form.

           - dot :: clustesr are written in GraphViz "dot" format.

           See "prefix" for file-in-stream naming conventions.
        */
        std::string m_format{"json"}; // "json" or "numpy"

        /**
           prefix = "cluster"

           Files in the output stream are all named with this prefix
           in order to allow other sinks to use the same stream.  

           In case of "json" file format the files will be named as:

           <prefix>_<clusterID>.json

           The "numpy" format exposes some of the graph complexity at
           the file level and so has a more complex naming scheme.  A
           single ICluster is serialized to a set of Numpy array each
           in its own .npy file.  There may also arrays holding
           information relevant to multiple IClusters (eg wire
           geometry).

           The structure assumes that each node or edge is of a "type"
           and that there is no variation in the schema for attributes
           on the nodes or edges within their "type".

           The nodes and edges are then given an index such that the
           indices for one type are contiguous.  The subsets of nodes
           or edges of a given type may then be located with small
           2-element arrays:

           <prefix>_<clusterID>_node_<type>.npy
           <prefix>_<clusterID>_edge_<type>.npy
           
           Each hold an (offset, size) pair defining the range of the
           node or edge indices of the given type.

           Zero or more N-D arrays may then be given for node or edge
           attributes of a given type


           <prefix>_<clusterID>_node_<type>_<attrname>.npy
           <prefix>_<clusterID>_edge_<type>_<attrname>.npy

           Interpretation of these arrays depends on out-of-band
           understanding based on their names as described next.

           Node types and their attributes.

           - channel :: ident, signal
           - blob :: ident, signal, layers
           - meas :: value
           - slice :: ident, duration

           With

           - ident :: the object's ident() number

           - signal :: a (value, uncertainty) pair

           - layers :: a (5,2) array of wire-in-plane (WIP) indices

           - duration :: a pair of times, (start,span) 

           Where these reference a "wire" they do so as an index in
           one or more arrays

           <prefix>_<clusterID>_wires_<type>>.npy

           With types

           - address :: (Nwire,5) with columns of (ident, planeid, WIP, chanID, segment)

           - rays :: (Nwire,2,3) with each (2,3) being (tail,head) of
             3-vector endpoints of wire segment, pointing away from
             channel (eg in direction of increasing segment number).

         */
        std::string m_prefix{"cluster"};

        // maybe add an "exclude" option identifying arrays to not
        // write.  


      public:
        // The output stream and serializer types
        using ostream_t = boost::iostreams::filtering_ostream;

      private:
        // internal implmentation of the actual serializer.
        using serializer_t = std::function<void(const ICluster& cluster)>;

        ostream_t m_out;
        serializer_t m_serializer;

        size_t m_count{0};

        std::string fqn(const ICluster& cluster, std::string name, std::string ext);
        void jsonify(const ICluster& cluster);
        void dotify(const ICluster& cluster);
        void numpify(const ICluster& cluster);

        template<typename ArrayType>
        void write_numpy(const ArrayType& arr, const std::string& name) {
            // log->debug("write {} ndim={} size={}", name, arr.num_dimensions(), arr.num_elements());
            Stream::write(m_out, name, arr);
            m_out.flush();
        }

    };

}  // namespace WireCell
#endif
