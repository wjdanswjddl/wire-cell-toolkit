#include <tbb/tbb.h>
#include <iostream>

int main() {
    tbb::flow::graph graph;

    // Define nodes
    tbb::flow::function_node<int, int> node1(graph, tbb::flow::unlimited, [](const int& input) {
        std::cout << "node 1 input: " << input << std::endl;
        // Perform computation for node1
        return input * 2;
    });

    tbb::flow::function_node<int, int> node2(graph, tbb::flow::unlimited, [](const int& input) {
        std::cout << "node 2 input: " << input << std::endl;
        // Perform computation for node2
        return input + 5;
    });

    // Define edges

    tbb::flow::make_edge(node1, node2);

    // Add input to the graph
    const int limit = 1;
    int count = 0;
    tbb::flow::input_node<int> src( graph, [&]( tbb::flow_control &fc ) -> int {
        std::cout << "src input" << std::endl;
        if (count++ < limit) {
            return count + 10;
        }
        fc.stop();
        return {};
    });

    tbb::flow::make_edge(src, node1);

    std::cout << "Running graph" << std::endl;
    // Wait for all nodes to finish
    src.activate();
    graph.wait_for_all();

    return 0;
}
