/** Sample RayGrid blobs
 */
#ifndef WIRECELLUTIL_RAYSAMPLERS
#define WIRECELLUTIL_RAYSAMPLERS

#include "WireCellUtil/RayTiling.h"

#include  <vector>

namespace WireCell::RayGrid {

    /** Sample blobs
     *
     * The API is uniform in that each method takes a RayGrid::Blob
     * and returns a vector of points sampling the blob with a given
     * strategy.  Some methods take additional parameters which are
     * set on the class as a whole.
     */
    class BlobSamplers {

        const Coordinates& coords;

      public:
        
        BlobSamplers(const Coordinates& coords) : coords(coords) {}
        
        /// fixme: configuration.  

        // The minimium number of wires over which a step will be
        // made.
        double min_step_size=3;
        // The maximum fraction of a blob a step may take.  If
        // non-positive, then all steps are min_step_size.
        double max_step_fraction = 1.0/12.0;

        using points_t = std::vector<Point>;

        points_t sample(const RayGrid::Blob& blob,
                        const std::string& strategy)
        {
            if (strategy == "center") {
                return center(blob);
            }
            if (strategy == "corners") {
                return corners(blob);
            }
            if (strategy == "edges") {
                return edges(blob);
            }
            if (strategy == "stepped") {
                return stepped(blob);
            }
            return points_t{};
        }

        // Return collection of a single point at center of blob corners
        points_t center(const RayGrid::Blob& blob)
        {
            points_t pts(1);
            const auto& corners = blob.corners();
            const size_t ncorners = corners.size();
            if (!ncorners) {
                return pts;
            }
            for (const auto& [one, two] : corners) {
                pts[0] += coords.ray_crossing(one, two);
            }
            pts[0] = pts[0] / ncorners;
            return pts;
        }

        // Return blob corner points 
        points_t corners(const RayGrid::Blob& blob)
        {
            points_t pts;
            const auto& corners = blob.corners();
            const size_t ncorners = corners.size();
            if (! ncorners) {
                return pts;
            }

            /// This does a sort which is not required.
            // pts = coords.ring_points(corners);
            /// so instead DIY for a little more speed.

            for (const auto& [one, two] : corners) {
                pts.push_back(coords.ray_crossing(one, two));
            }
            return pts;
        }

        // Return points on blob boundary edge centers
        points_t edges(const RayGrid::Blob& blob)
        {
            // need the sort to pair up corners
            auto pts = coords.ring_points(blob.corners());
            const size_t npts = pts.size();

            // walk around the ring of points, find midpoint of each edge.
            points_t points;
            for (size_t ind1=0; ind1<npts; ++ind1) {
                size_t ind2 = (1+ind1)%npts;
                const auto& origin = pts[ind1];
                const auto& egress = pts[ind2];
                const auto mid = 0.5*(egress + origin);
                points.push_back(mid);
            }
            return points;
        }

        // Sample on 2-view (min/max) grid with steps.

        points_t stepped(const RayGrid::Blob& blob)
        {

            auto strips = blob.strips();

            auto swidth = [](const Strip& s) -> int {
                return s.bounds.second - s.bounds.first;
            };
            std::sort(strips.begin()+2, strips.end(),
                      [&](const Strip& a, const Strip& b) -> bool {
                          return swidth(a) < swidth(b);
                      });
            const Strip& smin = strips[2];
            const Strip& smid = strips[3];
            const Strip& smax = strips[4];
        
            // both max
            int nmin = std::max(min_step_size, max_step_fraction*swidth(smin));
            int nmax = std::max(min_step_size, max_step_fraction*swidth(smax));

            points_t points;

            for (auto gmin=smin.bounds.first; gmin < smin.bounds.second; gmin += nmin) {
                coordinate_t cmin{smin.layer, gmin};
                for (auto gmax=smax.bounds.first; gmax < smax.bounds.second; gmax += nmax) {
                    coordinate_t cmax{smax.layer, gmax};
                
                    const double pitch = coords.pitch_location(cmin, cmax, smid.layer);
                    auto gmid = coords.pitch_index(pitch, smid.layer);
                    if (smid.in(gmid)) {
                        auto pt = coords.ray_crossing(cmin, cmax);
                        points.push_back(pt);
                    }
                }
            }
            return points;
        }

        // grid()
        // bounds()
    
    };    




}


#endif
