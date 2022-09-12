#include "boost/icl/interval_map.hpp"
#include "boost/icl/split_interval_map.hpp"
#include "boost/icl/separate_interval_set.hpp"
#include "boost/icl/split_interval_set.hpp"

#include <iostream>
#include <vector>
#include <typeinfo>

template<typename Key, typename ISet = boost::icl::interval_set<Key>>
void doit_set()
{
    using interval_t = typename boost::icl::interval<Key>::interval_type;
    // using interval_set_t = typename boost::icl::interval_set<Key>;

    const std::vector<std::pair<Key,Key>> intervals = {
        {1,4}, {2,5}, {5,7}
    };
    ISet is;
    for (const auto& [f,s] : intervals) {
        is += interval_t::right_open(f,s);
    }
    std::cerr << is << std::endl;
}

template<typename Key, typename IMap = boost::icl::interval_map<Key, std::set<size_t>>>
void doit_map(bool common)
{
    using interval_t = typename IMap::interval_type;
    using set_t = std::set<size_t>;
    // using interval_map_t = typename boost::icl::interval_map<Key, set_t>;

    const std::vector<std::pair<Key,Key>> intervals = {
        {1,4}, {2,5}, {5,7}
    };
    IMap im;
    size_t count{0};
    for (const auto& [f,s] : intervals) {
        if (common) count = 0;
        im += std::make_pair(interval_t::right_open(f,s), set_t{count});
        ++count;
    }
    std::cerr << im << std::endl;
}

template<typename Key>
void doit_key()
{
    std::cerr << "key type: " << typeid(Key).name() << "\n";

    std::cerr << "interval_set joining:\n";
    doit_set<Key, boost::icl::interval_set<Key>>();
    std::cerr << "interval_set separating:\n";
    doit_set<Key, boost::icl::separate_interval_set<Key>>();
    std::cerr << "interval_set splitting:\n";
    doit_set<Key, boost::icl::split_interval_set<Key>>();

    std::cerr << "interval_map joining same values:\n";
    doit_map<Key, boost::icl::interval_map<Key, std::set<size_t>>>(true);
    std::cerr << "interval_map splitting same values:\n";
    doit_map<Key, boost::icl::split_interval_map<Key, std::set<size_t>>>(true);

    std::cerr << "interval_map joining different values:\n";
    doit_map<Key, boost::icl::interval_map<Key, std::set<size_t>>>(false);
    std::cerr << "interval_map splitting different values:\n";
    doit_map<Key, boost::icl::split_interval_map<Key, std::set<size_t>>>(false);

    

}

int main()
{
    doit_key<int>();
    // doit_key<float>();
}
