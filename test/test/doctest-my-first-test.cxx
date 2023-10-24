#include "WireCellUtil/doctest.h"
#include "WireCellUtil/BuildConfig.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/Exceptions.h"

using namespace WireCell;

void THROW_something()
{
    THROW(ValueError() << errmsg{"the message"});
}
void throw_something()
{
    throw std::runtime_error{"the message"};
}

TEST_CASE("my first test") {
    bool ok=true;
    INFO("ok set to ", ok);
    CAPTURE(ok);
    CHECK(1 + 1 == 2);          // does not halt testing
    REQUIRE(ok == true);        // halts testing
    SUBCASE("a test using above as existing context") {
        ok = false;
        WARN(ok);           // emits log, including any CAPTURE/INFO's
        CHECK(!ok);
    }
    SUBCASE("another test using copy of the context") {
        // subcase above does not change our 'ok' variable
        CHECK(ok);
    }
}
  
TEST_CASE("my second test") {
    spdlog::debug("Set SPDLOG_LEVEL=debug in shell to see this");

    // Approximate floating point comparison with optional control
    // over precision.
    CHECK(3.14 == doctest::Approx(22.0/7).epsilon(0.01));

    // We can check for substring
    REQUIRE(WIRECELL_VERSION == doctest::Contains("0.")); // will we ever hit 1.0?

    // Assert that a function throws an exceptin of a certain type.
    CHECK_THROWS_AS(THROW_something(), ValueError);

    // WCT adds stack trace to the what() text so it will be hard to
    // use this macro.  It can be used with std::exception.
    CHECK_THROWS_WITH(throw_something(), "the message");
}
