#include "WireCellUtil/PointCluster.h"
#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"

#include <unordered_map>
#include <algorithm>

using namespace WireCell::PointCloud;

TEST_CASE("empty blob is empty") {
    Blob empty;
    for (auto s : empty.sizes()) {
        CHECK(s == 0);
    }
    CHECK(empty.hash() == 0);
    CHECK(empty.empty());
    spdlog::debug("blob: {}", empty);
    spdlog::debug("blob hash: {}", (void*)std::hash<Blob>{}(empty));
}

TEST_CASE("blob set get") {
    using RT = Blob::region_type;

    Blob b{ {1,2}, {2,3}, {3,4}, {4,5} };
    CHECK(b.u() == RT{1,2});
    CHECK(b.v() == RT{2,3});
    CHECK(b.w() == RT{3,4});
    CHECK(b.t() == RT{4,5});
    spdlog::debug("blob: {}", b);
    spdlog::debug("blob hash: {}", (void*)std::hash<Blob>{}(b));
}

TEST_CASE("blob set") {
    
    Blob b1{ {1,2}, {2,3}, {3,4}, {4,5} };
    Blob b2{ {1,2}, {2,3}, {3,4}, {4,5} };
    Blob b3{ {2,1}, {2,3}, {3,4}, {4,5} };


    CHECK(b1 == b2);
    CHECK(b1.hash() == b2.hash());

    CHECK(b1 != b3);
    CHECK(b1.hash() != b3.hash());

    Blobset bs;
    CHECK(bs.order().empty());
    CHECK(bs.index().empty());

    CHECK(! bs.has(b1));
    CHECK(! bs.has(b1));

    auto i1 = bs(b1);
    CHECK(i1==0);
    CHECK(bs.order().size() == 1);
    CHECK(bs.index().size() == 1);

    auto i2 = bs(b2);           // should be unchanged
    CHECK(i2==0);
    CHECK(bs.index().size() == 1);
    CHECK(bs.order().size() == 2);

    auto i3 = bs(b3);           // novel blob
    CHECK(i3==1);
    CHECK(bs.index().size() == 2);
    CHECK(bs.order().size() == 3);

    auto io = bs.index_order();
    CHECK(io.size() == 3);

    auto co = bs.invert();
    CHECK(co.size() == 2);

    
}


TEST_CASE("blob hash") {
    using RT = Blob::region_type;

    const int maxn = 10;        // check a miniscule subspace
    std::vector<int> thou(maxn);
    std::iota(thou.begin(), thou.end(), 0);

    std::unordered_map<size_t, size_t> counter;

    // Utter insanity
    Blob blob;
    size_t nhashes=0;
    auto& regions = blob.regions();
    for (auto u0 : thou) {
        for (auto u1=u0+1; u1<maxn; ++u1) {
            regions[0] = RT{u0,u1};
            for (auto v0 : thou) {
                for (auto v1=v0+1; v1<maxn; ++v1) {
                    regions[1] = RT{v0,v1};
                    // spdlog::debug("{}/{} last: {}", counter.size(), nhashes, blob);
                    for (auto w0 : thou) {
                        for (auto w1=w0+1; w1<maxn; ++w1) {
                            regions[2] = RT{w0,w1};
                            for (auto t0 : thou) {
                                for (auto t1=t0+1; t1<maxn; ++t1) {
                                    regions[3] = RT{t0,t1};
                                    const size_t h = blob.hash();
                                    counter[h] += 1;
                                    ++nhashes;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    size_t ncol=0;
    for (const auto& [h,c] : counter) {
        if (c==1) {
            continue;
        }
        spdlog::debug("collision: {} x {}", (void*)h, c);
        ++ncol;
    }
    // I see ~2 Mhps on i7-4770K
    spdlog::debug("total collisions: {} out of {}", ncol, nhashes);
    CHECK(ncol == 0);
}

