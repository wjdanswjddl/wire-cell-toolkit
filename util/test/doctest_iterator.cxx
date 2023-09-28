#include "WireCellUtil/doctest.h"

#include <map>
#include <vector>
#include <string>

TEST_CASE("iterator back from end")
{
    std::vector<int> vi = {0,1,2};
    vi.push_back(4);
    auto vit = vi.end();
    --vit;
    CHECK(*vit == 4);

    std::map<int,std::string> mis = { {0,"zero"}, {1,"one"}, {2,"two"} };
    auto mit = mis.end();
    --mit;
    CHECK(mit->first == 2);
    CHECK(mit->second == "two");
}
