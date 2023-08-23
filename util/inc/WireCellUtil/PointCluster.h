#ifndef WIRECELL_POINTCLUSTER
#define WIRECELL_POINTCLUSTER

#include "WireCellUtil/RayGrid.h"
#include "WireCellUtil/Range.h"
#include "WireCellUtil/IndexedMultiset.h"
#include "WireCellUtil/PointCloud.h"

#include <vector>
#include <string>

namespace WireCell::PointCloud {

    /** A blob represents a region in 3-space bounded by ranges of
     * time and wires in three (u,v,w) views.
     *
     * Each bound is represented by a HALF-OPEN range along a
     * "dimension".  The dimensions are ordered: u, v, w and t.
     * Though each dimension is discrete and represtented by an int,
     * u,v,w are measured in wire indices and t in tick indices.
     *
     * In Cartesian space, the t dimension is orthogonal to each of
     * u,v,w but u,v,w, are not mutually orthogonal.
     */
    class Blob {

        std::vector<region_type> m_regions[4];
      public:
        using meaure_type = int;
        using region_type = Range::iter_pair<measure_type>;

        const std::vector<region_type>& regions() const { return m_regions; };

        // Wire-in-plane (WIP) regions per plane.
        const region_type& u() const { return regions[0]; }
        const region_type& v() const { return regions[1]; }
        const region_type& w() const { return regions[2]; }
        // Tick-span region
        const region_type& t() const { return regions[3]; }

        // Return true if the blob covers no region.
        bool empty() const;

        // Return the size in each of the four dimensions.
        std::vector<measure_type> sizes() const;


    };

    /** Return the "distance" between two blobs in each dimension.
     *
     * The distance in each dimension is signed with this
     * interpretations:
     *
     * - d<0 :: the blobs overlap and the value gives the distance of their intersection.
     * - d=0 :: the blobs touch, the low bound of one is coincident with the high bound of the other.
     * - d>0 :: the value gives the distance between the low bound of one and the high bound of another.
     */
    std::vector<Blob::measure_type> distance(const Blob& one, const Blob& two);

    /** A std::hash()'alike for blobs. */
    inline std::size_t blob_hash(const Blob& blob) {
        std::size_t h=0;
        for (const auto& reg : blob.regions()) {
            const auto hh = std::hash( (reg.first << 32) | reg.second );
            h = h ^ (hh << 1);
        }
        return h;
    }

    /** A set of blobs providing order, context and identity */
    using Blobset = IndexedMultiset<Blob, size_t, blob_hash>;

    /** A cluster maintains a blob set, point clouds and k-d trees.
     *
     * A cluster may be mutated but at the cost of invalidating caches
     * including global point clouds.
     *
     */
    class Cluster {
      public:

        Blobset() = default;
        ~Blobset() = default;

        /// Add copy of blob to the blobset, returning its index.
        size_t insert(const Blob& blob);

        /** Associate a named point cloud with a blob at given index.
         */
        void pointcloud(const std::string& name, size_t index, const Dataset& pc);

        using pc_filter = std::function<Dataset(const Dataset&)>;

        /** Create a global point cloud of the given gpc_name by
            applying a filter to each blob point cloud of the given
            bpc_name and appending.
         
            The new PC is returned and available later by the gpc_name.
         */
        Dataset& global_pointcloud(const std::string& gpc_name,
                                   const std::string& bpc_name,
                                   pc_filter filter = copy);

        /** Recall a global point cloud.
         */
        const Dataset& global_pointcloud(const std::string& gpc_name) const;
        Dataset& global_pointcloud(const std::string& gpc_name);

        /** Remove blob at index.

            This mutates the blobset and may trigger cache rebuild.
        */
        void remove(size_t index);

        /** Bulk remove.

            This mutates the blobset and may trigger cache rebuild at
            most once.
        */
        void remove(const std::vector<size_t>& indices);

        /** Access blobset. */
        const Blobset& blobset() const;

        /** Return a (pseudo) blob representing the envelope of blobs
            in each dimension.

            Note for holding the return by reference: when the blobset
            is mutated this value is not updated until this method is
            subsequently called.
        */
        const Blob& envelope() const;


        /** Return a assymetric set of blobs in this near the other.

            Nearness is achieved when the distance between the blobs
            in the time dimension and at least one view dimension is
            less than or equal to the coresponding value given in the
            "dist" vector.  The default requires blobs to be
            "touching" (or overlapping) which includes the case that
            two blobs are "diagonal" across time and a view dimension.
         
            See distance(const Blob& one, const Blob& two) for how
            distance is calcualted.
         */
        Blobset nearby(const Cluster& other,
                       const std::vector<Blob::measure_type> dist = {0,0,0});

      private:

        // Update caches due to mutation on blobset.
        void update();

        // The set of blobs.
        Blobset m_blobs;

        // The pseudo blob holding the bounding "box" of the blobset.
        // Only access via a call to envelope().  An
        // m_envelope.empty() indicates it needs recalcualtion.
        mutable Blob m_envelope;

        // We memorize these to reapply if caller mutates our blobset.
        struct GPCMaker {
            std::string gpc_name, bpc_name;
            pc_filter filter;
        };
        std::vector<GPCMaker> m_gpcmakers;

        // Each blob has named point clouds.
        using blob_pointclouds_t = std::map<std::string, Dataset>;
        std::vector<blob_pointclouds_t> m_bpcs;

        // Global point clouds.  On blobset mutation, these are
        // invalidated and the gpcmakers rerun.  It is mutable as the
        // trigger can happen in a const access.
        mutable blob_pointclouds_t m_gpcs;
    };

    /** Return the set of blobs from each cluster than are .
     
       Two blobs are considered to overlap if their mutual distance in
       each dimension is strictly greater than
     */
    Blobset overlap(const Cluster& one, const Cluster& two,
                    const std::vector<Blob::measure_type> = {0,0,0,0});

}

#endif

