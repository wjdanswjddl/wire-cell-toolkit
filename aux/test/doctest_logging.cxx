#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"
using spdlog::debug;
using spdlog::info;

TEST_CASE("aux logging")
{
    debug("this is a debug message");
}
