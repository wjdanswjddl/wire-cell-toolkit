#include <iostream>
#include <unordered_map>
#include <string>
#include <memory>  // std::shared_ptr
#include <vector>
#include <cassert>

struct IChan {
    typedef std::shared_ptr<const IChan> pointer;
    IChan(const int ident) : m_ident(ident) {}
    int m_ident{-1};
};

struct IdentHash {
    size_t operator()(const IChan::pointer p) const {
        return std::hash<int>()(p->m_ident);
    }
};

struct IdentEq {
    size_t operator()(const IChan::pointer p1, const IChan::pointer p2) const {
        return p1->m_ident == p1->m_ident;
    }
};

using map_t = std::unordered_map<IChan::pointer, std::string, IdentHash, IdentEq>;

void check(const map_t& m, const IChan::pointer p) {
    if (m.find(p) != m.end())
        std::cout << p << " " << p->m_ident << " " << IdentHash{}(p) << " " << m.at(p) << std::endl;
}

int main() {
    map_t myMap;

    auto p1 = IChan::pointer(new IChan(1));
    auto p3 = IChan::pointer(new IChan(3));
    auto p42 = IChan::pointer(new IChan(42));
    myMap[p1] = "One";
    myMap[p3] = "Three";
    myMap[p42] = "Fortytwo";
    
    check(myMap, p1);
    check(myMap, p3);
    
    // check the iteration order, it seems different for different machine
    // onlinegdb: https://onlinegdb.com/6VcfHUHE6
    std::cout << "check order 1, 3, 42:" << std::endl;
    std::vector<int> order1;
    for (const auto& [p, s] : myMap) {
        std::cout << p << " " << s << std::endl;
        order1.push_back(p->m_ident);
    }
    // assert(order1 == (std::vector<int>{42,3,1}) && "order1 should be {42,3,1}");
    if (order1 != (std::vector<int>{42,3,1})) {
        std::cout << "order1 != {42,3,1}" << std::endl;
    }
    myMap.clear();
    myMap[p3] = "Three";
    myMap[p42] = "Fortytwo";
    myMap[p1] = "One";
    std::cout << "check order: 3, 42, 1" << std::endl;
    std::vector<int> order2;
    for (const auto& [p, s] : myMap) {
        std::cout << p << " " << s << std::endl;
        order2.push_back(p->m_ident);
    }
    // assert(order2 == (std::vector<int>{1,42,3}) && "order1 should be {1,42,3}");
    if (order2 != (std::vector<int>{1,42,3})) {
        std::cout << "order2 != {1,42,3}" << std::endl;
    }
    
    // fetch using new pointers
    auto newp1 = IChan::pointer(new IChan(1));
    auto newp3 = IChan::pointer(new IChan(3));
    check(myMap, newp1);
    check(myMap, newp3);
    
    

    return 0;
}