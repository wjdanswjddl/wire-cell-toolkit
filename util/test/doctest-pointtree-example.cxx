#include "WireCellUtil/PointTree.h"

#include "WireCellUtil/PointTesting.h"
#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"

using namespace WireCell;
using namespace WireCell::PointCloud;
using namespace WireCell::PointCloud::Tree; // for "Points" node value type
using namespace WireCell::PointTesting;     // make_janky_track()
using namespace spdlog;                     // for debug() etc.

TEST_CASE("point tree example simple point cloud")
{
    Dataset pc({
        {"x", Array({1.0, 1.0, 1.0})},
        {"y", Array({2.0, 1.0, 3.0})},
        {"z", Array({1.0, 4.0, 1.0})}});
    
    // Each array is size 3 and thus the PC has that major axis size
    CHECK( pc.size_major() == 3 );
    
    // Accessing a single element in the PC takes two steps: get the array by
    // name, get the element by index.
    auto arr = pc.get("x");
    CHECK( arr );
    
    // We must supply a C++ type in order to extract a value.
    CHECK( arr->element<double>(0) == 1.0 );

Dataset::selection_t sel = pc.selection({"x","y","z"});

// The selection must be the same size as the list of names.
CHECK( sel.size() == 3 );

// Get first array ("x") and the value at its index 0.
CHECK( sel[0]->element<double>(0) == 1.0 );

using point_type = coordinate_point<double>;

// Create a coordinate point on the selection and at a given point index.
point_type cpt(&sel, 0);

// The index in the point cloud can be retrieved.
CHECK( cpt.index() == 0 );

// The point looks like a vector of size 3.
CHECK( cpt.size() == 3);

// And we can access the "column" at the current index by a dimension number.
CHECK( cpt[0] == 1.0 );           // x
CHECK( cpt[1] == 2.0 );           // y
CHECK( cpt[2] == 1.0 );           // z

// If the coordinate point is not const, it can be set.
cpt.index() = 1;
CHECK( cpt.index() == 1 );

// We may also access the dimensions of the point with bounds checking on
// both index and dimension.
CHECK( cpt.at(0) == 1.0 );        // x
CHECK( cpt.at(1) == 1.0 );        // y
CHECK( cpt.at(2) == 4.0 );        // z

using coords_type = coordinate_range<point_type>;

    // Make a coordinate range on the selection.
    coords_type cr(sel);

    // Iterate over the selection, point by point.
    for (const auto& cpt : cr) {
        // Each cpt is a "column" down the rows of selected arrays.
        CHECK( cpt.size() == sel.size() );
    }
}

using node_ptr = std::unique_ptr<Points::node_t>;

TEST_CASE("point tree example nodeless points")
{
    Points p( { {"3d", Dataset({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})}}) } });

    // Normally, we can get the node that holds this value but in this example
    // that node does not exist.
    CHECK( p.node() == nullptr );

    // But we can access the local point clouds.
    auto& lpcs = p.local_pcs();
    CHECK( lpcs.size() == 1 );
    CHECK( lpcs.find("3d") != lpcs.end() );
}

TEST_CASE("point tree example single node")
{
    // Normally, we would not create a node on the stack as it could never be a
    // child because as a child must be held by unique_ptr.  
    Points::node_t n(Points( { {"3d", Dataset({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})}}) } }));

    // The node directly exposes its value as a data member  
    Points& p = n.value;

    // And we can go full circle to get a pointer to the value's node.
    Points::node_t* nptr = p.node();

    // This time, that node must exist because we just made it
    CHECK( nptr == &n );
}

static
Points::node_ptr make_simple_pctree()
{
    // We will return this unique pointer to node
    Points::node_ptr root = std::make_unique<Points::node_t>();

    // Insert first child with a set of named points clouds containing one point
    // cloud build from a track.
    auto* n1 = root->insert(Points({ {"3d", make_janky_track()} }));
    REQUIRE(n1 != nullptr);

    // Insert a second child with a point cloud from a different track.
    auto* n2 = root->insert(Points({ {"3d", make_janky_track(
                        Ray(Point(-1, 2, 3), Point(1, -2, -3)))} }));
    REQUIRE(n2 != nullptr);

    return root;
}

// Loop over children and dump info about them.
static void dump_children(const node_ptr& node)
{
    // Loop over children node "Points" values 
    for (auto& cval : node->child_values()) {
        // The named point clouds held by this node value.
        const auto& pcs = cval.local_pcs();

        debug("child node at {} with {} local point clouds",
              (void*)cval.node(), pcs.size());

        // loop over the set of point clouds
        for (const auto& [name, pc] : pcs) {
            debug("\tchild has pc named \"{}\" with {} points",
                  name, pc.size_major());
        }
    }
}

TEST_CASE("point tree example simple tree operations")
{
    auto root = make_simple_pctree();
    dump_children(root);

    // Find iterator to first child in child list
    CHECK( root->children().size() == 2 );
    auto cit = root->children().begin();

    // Remove it as a child and we get ownership as unique_ptr.
    auto cuptr = root->remove(cit);
    CHECK( root->children().size() == 1 );

    // We can add that orphaned child back.  This transfers ownership.
    auto* cptr = root->insert(std::move(cuptr));
    CHECK( root->children().size() == 2 );

    // But, we caught the return and now have a loaned bare pointer
    CHECK( cptr );

    // We should now see the reverse order as above dump.
    dump_children(root);
}

TEST_CASE("point tree example scope")
{
    // A default scope can be constructed.  It is a null scope as it would match
    // no local point cloud arrays despite having unlimited depth.
    Scope s;
    CHECK( s.pcname == "" );
    CHECK( s.coords.empty() );
    CHECK( s.depth == 0 );

    // Some non-empty scopes.  Note the case sensitivity in all names.
    Scope s0{ "pcname", {"x","y","z"}, 0};
    Scope s1{ "pcname", {"x","y","z"}, 1};
    Scope s2{ "PCNAME", {"x","y","z"}, 0};
    Scope s3{ "pcname", {"X","Y","Z"}, 0};
    Scope sc{ "pcname", {"x","y","z"}, 0};

    // Scopes can be compared for equality
    CHECK( s0 == s0 );
    CHECK( s0 == sc );
    CHECK( s0 != s1 );
    CHECK( s0 != s2 );
    CHECK( s0 != s3 );

    // A scope has a std::hash().
    CHECK( s0.hash() == sc.hash() );
    CHECK( s0.hash() != s1.hash() );
    CHECK( s0.hash() != s2.hash() );
    CHECK( s0.hash() != s3.hash() );

    // So, it may be used as a key.
    std::unordered_map<Scope, size_t> m;
    m[s1] = 1;
    m[s2] = 2;
    CHECK( m[s0] == 0 );          // size_t default value
    CHECK( m[s1] == 1 );
    CHECK( m[s2] == 2 );

    // One can also print a scope
    debug("Here is a scope: {}", s0);
}

TEST_CASE("point tree example scoped point cloud")
{
    auto root = make_simple_pctree();

    // Specify point cloud name and the arrays to select.  We take the default
    // depth number of 0.
    Scope scope{ "3d", {"x","y","z"} };

    // The corresponding scoped view. 
    auto const & sv = root->value.scoped_view(scope);

    CHECK(sv.npoints() > 0);
}

TEST_CASE("point tree example scoped nodes")
{
    auto root = make_simple_pctree();

    // Specify point cloud name and the arrays to select.  We take the default
    // depth number of 0.
    Scope scope{ "3d", {"x","y","z"} };

    // The corresponding scoped view. 
    auto const & sv = root->value.scoped_view(scope);

    // Vector of pointers to n-ary tree nodes
    auto const& snodes = sv.nodes();
    CHECK(snodes.size() > 0);
    for (const auto& node : snodes) {
        const auto& lpcs = node->value.local_pcs();
        REQUIRE(node);
        CHECK(lpcs.find("3d") != lpcs.end());
    }
}

TEST_CASE("point tree example scoped nodes")
  {
      auto root = make_simple_pctree();

      // Specify point cloud name and the arrays to select.  We take the default
      // depth number of 0.
      Scope scope{ "3d", {"x","y","z"} };

      // The corresponding scoped view, its nodes and point clouds
      auto const & sv = root->value.scoped_view(scope);
      auto const& snodes = sv.nodes();
      auto const& spcs = sv.pcs();
      CHECK(spcs.size() == snodes.size());

      // A scoped point cloud is a vector of references to point clouds
      for (const Dataset& pc : spcs) {
          debug("pc {} arrays and {} points",
                pc.size(), pc.size_major());
      }
}

#include "WireCellUtil/DisjointRange.h"
TEST_CASE("point tree example scoped point cloud disjoint")
{
    // As above.
    auto root = make_simple_pctree();
    auto spc = root->value.scoped_view(Scope{ "3d", {"x","y","z"} }).pcs();

    size_t npoints = 0;
    for (const Dataset& pc : spc) {
        npoints += pc.size_major();
    }

    // This part is a bit verbose, see text.
    // First we select a subset of arrays from each point cloud.
    std::vector<Dataset::selection_t> sels;
    for (Dataset& pc : spc) {
        sels.push_back(pc.selection({"x","y","z"}));
    }
    // Then we wrap them as coordinate points.
    using point_type = coordinate_point<double>;
    using point_range = coordinate_range<point_type>;
    std::vector<point_range> prs;
    for (auto& sel : sels) {
        prs.push_back(point_range(sel));
    }
    // Finally we join them together as a disjoint range
    using points_t = disjoint_range<point_range>;
    points_t points;
    for (auto& pr : prs) {
        points.append(pr.begin(), pr.end());
    }

    // Finally, we can perform a simple iteration as if we had a single
    // contiguous selection.
    CHECK( points.size() == npoints );

    for (auto& pt : points) {
        CHECK( pt.size() == 3);
    }

    // We know the scoped PC has two local PCs.  Pick an index in either
    const size_t in0 = 0.25*npoints; // in local PC at index 0
    const size_t in1 = 0.75*npoints; // in local PC at index 1

    // Iterator to the first point
    auto beg = points.begin();

    // Iterators to points inside the flat disjoint_range.  Eg, as we will see
    // returned later from a k-d tree query.
    points_t::iterator mit0 = beg + in0;
    points_t::iterator mit1 = beg + in1;

    // Full circle, find that these points are provided by the first and second
    // major range.
    size_t maj0 = points.major_index(mit0);
    size_t maj1 = points.major_index(mit1);

    CHECK( 0 == maj0 );
    CHECK( 1 == maj1 );
}

TEST_CASE("point tree example scoped k-d tree")
{
    auto root = make_simple_pctree();

    // Form a k-d tree query over a scoped point cloud.
    Scope scope = { "3d", {"x","y","z"} };
    const auto& sv = root->value.scoped_view(scope);
    const auto& skd = sv.kd();

    // Some query point.
    const std::vector<double> origin = {0,0,0};

    // Find three nearest neighbors.
    auto knn = skd.knn(3, origin);
    CHECK( knn.size() == 3 );

    // Get the underlying scoped PC.
    const auto& spc = sv.pcs();

    // Loop over results and refrence back to original scoped PC at both major
    // and minor range level.
    for (size_t pt_ind = 0; pt_ind<knn.size(); ++pt_ind) {
        auto& [pit,dist] = knn[pt_ind];

        const size_t maj_ind = skd.major_index(pit);
        const size_t min_ind = skd.minor_index(pit);
        debug("knn point {} at distance {} from query is in local point cloud {} at index {}",
              pt_ind, dist, maj_ind, min_ind);
        const Dataset& pc = spc[maj_ind];
        for (const auto& name : scope.coords) {
            debug("\t{} = {}", name, pc.get(name)->element<double>(min_ind));
        }
    }
}

// a little helper
static const Dataset& get_local_pc(const Points& pval, const std::string& pcname)
{
    const auto& pcs = pval.local_pcs();
    auto pcit = pcs.find(pcname);
    if (pcit == pcs.end()) {
        raise<KeyError>("no pc named " + pcname);
    }
    return pcit->second;
}

TEST_CASE("point tree example scoped k-d tree to n-ary nodes")
{
    auto root = make_simple_pctree();

    // Form a k-d tree query over a scoped point cloud.
    Scope scope = { "3d", {"x","y","z"} };


    // Get the scoped view parts
    const auto& sv = root->value.scoped_view(scope);
    const auto& skd = sv.kd();
    const auto& snodes = sv.nodes();

    // Some query point.
    const std::vector<double> origin = {0,0,0};

    // Find three nearest neighbors.
    auto knn = skd.knn(3, origin);
    CHECK( knn.size() == 3 );

    // Loop over results and refrence back to original scoped PC at both major
    // and minor range level.
    for (size_t pt_ind = 0; pt_ind<knn.size(); ++pt_ind) {
        auto& [pit,dist] = knn[pt_ind];

        const size_t maj_ind = skd.major_index(pit);

        const auto* node = snodes[maj_ind];
        debug("knn point {} at distance {} from query at node {} with {} children",
              pt_ind, dist, maj_ind, node->children().size());
    }
}
