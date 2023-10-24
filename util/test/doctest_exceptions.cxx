#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Type.h"
#include "WireCellUtil/doctest.h"

#include <iostream>

using namespace WireCell;


TEST_CASE("raise with empty string")
{
    CHECK_THROWS_AS(raise<ValueError>(), Exception);
}

TEST_CASE("raise with just string")
{
    CHECK_THROWS_AS(raise<ValueError>("just a string"), ValueError);
}

TEST_CASE("raise with format arguments")
{
    CHECK_THROWS_AS(raise<ValueError>("a fmt like thing %d", 42), ValueError);
}

TEST_CASE("catch as WCT exception base")
{
    CHECK_THROWS_AS(raise<IndexError>(), Exception);
}

TEST_CASE("catch as std exception base diy")
{
    try {
        raise<IndexError>();
    }
    catch (const std::exception& err) {
    }
}


TEST_CASE("catch as std exception base with doctest")
{
    CHECK_THROWS_AS(raise<IndexError>(), const std::exception&);
}

TEST_CASE("raise with format code error")
{
    CHECK_THROWS_AS(raise<ValueError>("let's break formatting %f", "a string"), ValueError);
}
