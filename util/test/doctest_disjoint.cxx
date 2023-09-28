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
    using value_type = int;
    using inner_vector = std::vector<value_type>;
    using outer_vector = std::vector<inner_vector>;
    using disjoint_type = disjoint<outer_vector::iterator>;
    outer_vector hier;

    {
        auto dj = disjoint_type(hier.begin(), hier.end());
        CHECK(dj.begin() == dj.end()); // because hier is empty
    }
    {
        auto dj = flatten(hier);
        CHECK(dj.begin() == dj.end()); // because hier is empty
    }    
}

void test_sequence(std::vector<std::vector<int>> hier,
                   std::vector<int> want)
{
    auto numbers = flatten(hier);
    REQUIRE(numbers.end() == numbers.end());
    {
        REQUIRE(numbers.size() == want.size());
        auto beg = numbers.begin();
        auto end = numbers.end();
        REQUIRE(beg != end);
        REQUIRE(std::distance(beg, end) == want.size());
    }
    {
        auto n2 = numbers;
        REQUIRE(n2.size() == want.size());
        REQUIRE(n2.begin() == numbers.begin());
    }
    {
        auto it = numbers.begin();
        REQUIRE(it != numbers.end());
        it += want.size();
        debug("jump to {} now at distance {}", want.size(), std::distance(it, numbers.end()));
        REQUIRE(it == numbers.end());
    }
    {
        auto it = numbers.begin();
        for (size_t left=want.size(); left; --left) {
            REQUIRE(it != numbers.end());
            ++it;
        }
        REQUIRE(it == numbers.end());        
    }


    debug("flatten test sequence of native outer size={}", hier.size());
    size_t ind = 0;
    for (const auto& num : numbers) {
        CHECK(ind < 5);
        debug("dji: ind={} want={} got={}", ind, want[ind], num);
        CHECK(num == want[ind]);
        ++ind;
    }
    debug("flatten test sequence of native outer size={}", hier.size());
    auto dj = flatten(hier);
    debug("manual stepping of iterator");
    auto djit = dj.begin();     // 0
    auto end = dj.end();
    CHECK(djit == dj);
    ++djit;                     // 1
    CHECK(djit != dj);
    CHECK(*djit == want[1]);
    --djit;                     // 0
    CHECK(*djit == want[0]);
    djit += 2;                  // 2
    CHECK(*djit == want[2]);    
    djit -= 2;                  // 0
    CHECK(*djit == want[0]);
    debug("adding {} to begin", want.size());
    djit += want.size();        // 5
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
    auto djit = flatten(hier);

    CHECK(*djit == 0);
    *djit = 42;
    CHECK(*djit == 42);
    CHECK(hier[0][0] == 42);
}

TEST_CASE("disjoint iterator const") {
    using vi = std::vector<int>;
    using vvi = std::vector<vi>;

    using dji_vvivc = disjoint<vvi::iterator, vi::const_iterator>;
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

    // using dji_vvcvc = disjoint<vvi::const_iterator, vi::const_iterator>;
    // auto vvcvc = dji_vvcvc(hier.cbegin(), hier.cend());
    // ++vvcvc;

}
