#include "WireCellUtil/Testing.h"
#include "WireCellUtil/IndexedMultiset.h"
#include "WireCellUtil/doctest.h"

using namespace WireCell;

TEST_CASE("indexed multiset basics")
{

    IndexedMultiset<int> imsi;
    CHECK(imsi(42) == 0);
    CHECK(imsi(69) == 1);
    CHECK(imsi(42) == 0);
    CHECK(imsi(0) == 2);
    CHECK(imsi(0) == 2);
    CHECK(imsi(69) == 1);

    CHECK(imsi.has(0));
    CHECK(!imsi.has(1));
    CHECK(imsi.has(42));
    CHECK(imsi.has(69));

    auto ind = imsi.index();
    auto ord = imsi.order();

    CHECK(ind.size() == 3);
    CHECK(ord.size() == 6);
    
    auto inv = imsi.invert();
    CHECK(inv.size() == 3);
    CHECK(inv[0] == 42);
    CHECK(inv[1] == 69);
    CHECK(inv[2] == 0);

    auto ino = imsi.index_order();
    CHECK(ino.size() == 6);
    CHECK(ino[0] == 0);
    CHECK(ino[1] == 1);
    CHECK(ino[2] == 0);
    CHECK(ino[3] == 2);
    CHECK(ino[4] == 2);
    CHECK(ino[5] == 1);
    
}
