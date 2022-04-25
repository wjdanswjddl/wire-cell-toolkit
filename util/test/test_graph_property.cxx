#include "WireCellUtil/GraphTools.h"

#include <string>
#include <vector>
#include <iostream>

// namespace helper {

// template <typename It> boost::iterator_range<It> mir(std::pair<It, It> const& p) {
//     return boost::make_iterator_range(p.first, p.second);
// }
// template <typename It> boost::iterator_range<It> mir(It b, It e) {
//     return boost::make_iterator_range(b, e);
// }
// template <typename Gr> 
// boost::iterator_range<typename boost::graph_traits<Gr>::vertex_iterator> vertex_range(Gr& g) {
//     return mir(boost::vertices(g));
// }
// }

struct node_t {
    int count;
    float value;
};


using graph_t = boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS, node_t>;
using vdesc_t = boost::graph_traits<graph_t>::vertex_descriptor;

using namespace WireCell;

int main()
{
    graph_t gr;
    int count=0;
    auto v1 = boost::add_vertex(node_t{count++, 42}, gr);
    auto v2 = boost::add_vertex(node_t{count++, 24}, gr);
    auto [e,ok] = boost::add_edge(v1, v2, gr);
    std::cerr << "edge: " << e << " " << ok << "\n";

    std::vector<std::string> string_property{"one", "two"};
    for (const auto& vtx : GraphTools::vertex_range(gr)) {
        std::cerr << "vtx: " << vtx
                  << " " << string_property[vtx] << "\n";
    }
    return 0;
}
