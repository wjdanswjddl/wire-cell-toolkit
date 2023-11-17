#include "WireCellUtil/Logging.h"
#include "WireCellUtil/Type.h"
#include "WireCellUtil/doctest.h"

#include <iostream>
#include <vector>
using namespace WireCell;

using vi_t = std::vector<int>;

TEST_CASE("typeid and demangling")
{
    int i;
    vi_t vi;
    CHECK(typeid(int) == typeid(i));
    CHECK(type(vi) == type<vi_t>());
    CHECK("std::vector<int, std::allocator<int> >" == type(vi));
}

