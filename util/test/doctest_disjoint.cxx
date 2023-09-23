/** Test the Disjoint API.
 */

#include "WireCellUtil/Disjoint.h"
#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"

using spdlog::debug;
using namespace WireCell;

TEST_CASE("disjoint iterator test stl")
{

    {
        std::map<int, int> m = { {0,0}, {1,1}, {2,2} };
        auto mit = m.begin();
        CHECK(mit->second == 0);
        ++mit;
        CHECK(mit->second == 1);
        --mit;
        CHECK(mit->second == 0);
        mit = m.end();
        --mit;
        CHECK(mit->second == 2);
    }

    {
        std::vector<int> want = {0,1,2,3,4};
        auto vit = want.end();
        debug("test_sequence: go one before end");
        --vit;
        debug("test_sequence: gone one before end");
        CHECK(*vit == want.back());
        debug("test_sequence: gone one before end okay");
    }
}


TEST_CASE("disjoint iterator simple") {
    std::vector<std::vector<int>> hier;
    auto di1 = flatten(hier.begin(), hier.end());
    auto di2 = flatten(hier.end());
    CHECK(di1 == di2);
}

void test_sequence(std::vector<std::vector<int>> hier,
                   std::vector<int> want)
{
    size_t ind = 0;
    auto end = flatten(hier.end());
    for (auto i = flatten(hier.begin(), hier.end()); i != end; ++i) {
        CHECK(ind < 5);
        debug("dji: ind={} want={} got={}", ind, want[ind], *i);
        CHECK(*i == want[ind]);
        ++ind;
    }


    auto djit = disjoint_iterator(hier.begin(), hier.end());
    ++djit;
    CHECK(*djit == want[1]);
    --djit;
    CHECK(*djit == want[0]);
    djit += 2;
    CHECK(*djit == want[2]);    
    djit -= 2;
    CHECK(*djit == want[0]);
    djit += want.size();
    CHECK(djit == end);
    debug("try backwards from end");
    --djit;                     // backwards from end
    debug("survived, now check equality");
    CHECK(*djit == want.back());
    debug("try backwards from last to first");
    djit -= want.size() - 1;
    debug("made it, now checking equality with front");
    CHECK(*djit == want.front());
    debug("still okay, let's try one step before begin and assure that throws");
    CHECK_THROWS_AS(--djit, std::runtime_error);
}

TEST_CASE("disjoint iterator no empties") {
    test_sequence({ {0,1,2}, {3}, {4} }, {0,1,2,3,4});
}
TEST_CASE("disjoint iterator with empties") {
    test_sequence({ {}, {0,1,2}, {}, {3}, {4}, {}}, {0,1,2,3,4});
}
TEST_CASE("disjoint iterator mutate") {
    std::vector<std::vector<int>> hier = { {0,1,2}, {3}, {4} };
    auto djit = disjoint_iterator(hier.begin(), hier.end());

    CHECK(*djit == 0);
    *djit = 42;
    CHECK(*djit == 42);
    CHECK(hier[0][0] == 42);
}

TEST_CASE("disjoint iterator const") {
    using vi = std::vector<int>;
    using vvi = std::vector<vi>;

    using dji_vvivc = disjoint_iterator<vvi::iterator, vi::const_iterator>;
    vvi hier = { {0,1,2}, {3}, {4} };
  
    auto vvivc = dji_vvivc(hier.begin(), hier.end());
    ++vvivc;
    int val = *vvivc;
    CHECK(val == 1);

    // should not compile!
    int& ref2 = *vvivc;
    CHECK(ref2 == 1);

    int const& ref = *vvivc;
    CHECK(ref == 1);

    // using dji_vvcvc = disjoint_iterator<vvi::const_iterator, vi::const_iterator>;
    // auto vvcvc = dji_vvcvc(hier.cbegin(), hier.cend());
    // ++vvcvc;

}
