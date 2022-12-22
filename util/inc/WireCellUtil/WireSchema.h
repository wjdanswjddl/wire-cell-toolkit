/**
   WireSchema describe "wire geometry" data structures.

   It provides a loader of WCT "wires file" following the "wire data
   schema" (typically as compressed JSON).

   The content must adhere to a number of required conventions
   described in util/docs/wire-schema.org.

   See also the Python module wirecell.util.wires.schema and family of
   CLI commands: "wirecell-util convert-*".
 */
#ifndef WIRECELLUTIL_WIREPLANES
#define WIRECELLUTIL_WIREPLANES

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/BoundingBox.h"
#include <memory>

namespace WireCell {

    namespace WireSchema {

        // IWire
        struct Wire {
            int ident;
            int channel;
            int segment;
            Point tail, head;  // end points, direction of signal to channel
        };

        struct Plane {
            int ident;
            std::vector<int> wires;
        };

        struct Face {
            int ident;
            std::vector<int> planes;
        };

        struct Anode {
            int ident;
            std::vector<int> faces;
        };

        struct Detector {
            int ident;
            std::vector<int> anodes;
        };

        struct StoreDB {
            std::vector<Detector> detectors;
            std::vector<Anode> anodes;
            std::vector<Face> faces;
            std::vector<Plane> planes;
            std::vector<Wire> wires;
        };

        // Access store via shared pointer to allow for caching of underlying data.
        typedef std::shared_ptr<const StoreDB> StoreDBPtr;

        // Bolt on some const functions to the underlying and shared store.
        class Store {
            StoreDBPtr m_db;

          public:
            Store();  // underlying store will be null!
            Store(StoreDBPtr db);
            Store(const Store& other);  // copy ctro
            Store& operator=(const Store& other);

            // Throw ValueError if store violates required conventions.
            void validate() const;

            // Access underlying data store as shared pointer.
            StoreDBPtr db() const;

            const std::vector<Detector>& detectors() const;
            const std::vector<Anode>& anodes() const;
            const std::vector<Face>& faces() const;
            const std::vector<Plane>& planes() const;
            const std::vector<Wire>& wires() const;

            const Anode& anode(int ident) const;

            std::vector<Anode> anodes(const Detector& detector) const;
            std::vector<Face> faces(const Anode& anode) const;
            std::vector<Plane> planes(const Face& face) const;

            // Return ordered wires.
            std::vector<Wire> wires(const Plane& plane) const;

            // Return the smallest axis-aligned box bounding the
            // object.  If region is true, the box bounds the wire
            // regions and not just the wire centers.  Wire-center
            // bounds are returned by default.
            BoundingBox bounding_box(const Anode& anode, bool region=false) const;
            BoundingBox bounding_box(const Face& face, bool region=false) const;
            BoundingBox bounding_box(const Plane& plane, bool region=false) const;

            // Return wire and pitch direction unit vectors.  This
            // tries to be robust in the face of imprecise wire data.
            Ray wire_pitch(const Plane& plane) const;

            // Return pitch distance vector averaged over wires in plane.
            Vector mean_pitch(const Plane& plane) const;

            // Return wire distance vector averaged over wires in plane.
            Vector mean_wire(const Plane& plane) const;

            /**
               Return vector of ray pairs that categorize each layer
               of a face as suitable for use in constructing a
               RayGrid::Coordinates.
            
               First two elements are horizontal and vertical bounds
               and subsequent elements bound wire0 region for each
               wire plane in the face.
            */
            ray_pair_vector_t ray_pairs(const Face& face) const;

            /**
               Return ray pairs bounding horiz/vert active area.
            */
            ray_pair_vector_t ray_pairs_active(const Face& face) const;

            std::vector<int> channels(const Plane& plane) const;
        };

        /**
           Load WCT "wires file".

           A number of cumulative corrections can be applied.

           - none :: The store represents data as-is from the file.

           - order :: Correct order of wire-in-plane (WIP) and wire
             endpoints.

             Normally, this orders by ascending wire center Z value
             but in the case wires are nearly parallel with the Z axis
             this orders by ascending wire center Y value.

             Normally, this orders (tail,head) endpoints by ascending
             Y value but in the case of Z-parallel wires (tail,head)
             is ordered by DEscending Z.

             These ordering rules assure that the diretion of
             increasing WIP is same as increasing pitch vector and
             that X x W = P.

           - direction :: Rotate wires about their centers so that
             they are all parallel.  The common wire direction is
             taken as the average over the original wires, rotated
             into the Y-Z plane.  Wire length and centers are held
             fixed.

           - pitch :: Translate wires along a common pitch direction
             so that they become uniformly distributed.  The common
             pitch is taken as the average over all wires rotated into
             the Y-Z plane.  The center Y/Z of the central wire at WIP
             = nwires/2 is kept fixed and X is set to the average of
             all center X.
           
        */
        enum struct Correction { empty, load, order, direction, pitch, ncorrections };
        inline Correction& operator++(Correction& c) {
            const int in = static_cast<int>(c) + 1;
            return c = static_cast<Correction>( in );
        }
        inline Correction& operator--(Correction& c) {
            const int in = static_cast<int>(c) - 1;
            return c = static_cast<Correction>( in );
        }

        // Load file into store performing correction
        Store load(const char* filename, Correction correction = Correction::pitch);

        // Dump store to file
        void dump(const char* filename, const Store& store);

    }  // namespace WireSchema

}  // namespace WireCell
#endif
