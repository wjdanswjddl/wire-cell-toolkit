#include "WireCellUtil/Testing.h"
#include "WireCellUtil/IndexedSet.h"
#include "WireCellUtil/doctest.h"

using namespace WireCell;

TEST_CASE("indexed set basics")
{
    IndexedSet<int> isi;
    CHECK(isi(42) == 0);
    CHECK(isi(69) == 1);
    CHECK(isi(42) == 0);
    CHECK(isi(0) == 2);
    CHECK(isi(0) == 2);
    CHECK(isi(69) == 1);
    CHECK(isi.collection.size() == 3);
}
