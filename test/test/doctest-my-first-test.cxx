#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"
TEST_CASE("my first test") {
    bool ok=true;
    CHECK(1 + 1 == 2);          // does not halt testing
    REQUIRE(ok == true);        // halts testing
    SUBCASE("a test using above as existing context") {
        ok = false;
        CHECK(!ok);
    }
    SUBCASE("another test using copy of the context") {
        // subcase above does not change our 'ok' variable
        CHECK(ok);
    }
}
  
TEST_CASE("my second test") {
    spdlog::debug("this test is very trivial");
    CHECK(true);
}
