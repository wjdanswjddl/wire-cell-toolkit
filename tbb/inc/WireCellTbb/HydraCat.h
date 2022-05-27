/**
   The hydra category adapts a WCT IHydraNodeBase to TBB.  

   The TBB subgraph uses an indexer_node on input and a
   multifunction_node on output. 

 */

#ifndef WIRECELLTBB_HYDRACAT
#define WIRECELLTBB_HYDRACAT

#include "WireCellIface/IHydraNode.h"
#include "WireCellTbb/NodeWrapper.h"

namespace WireCellTbb {

    template<typename T, unsigned N, typename... REST>
    struct generate_indexer_node
    {
        typedef typename generate_indexer_node<T, N-1, T, REST...>::type type;
    };
    template<typename T, typename... REST>
    struct generate_indexer_node<T, 0, REST...> {
        typedef tbb::flow::indexer_node<T, REST...> type;
    };

    template<typename InT, typename T, unsigned N, typename... REST>
    struct generate_outdexer_node
    {
        typedef typename generate_outdexer_node<InT, T, N-1, T, REST...>::type type;
    };
    template<typename InT, typename T, typename... REST>
    struct generate_outdexer_node<InT, T, 0, REST...> {
        typedef tbb::flow::multifunction_node<InT, std::tuple<T, REST...>> type;
    };


    template <typename IndexerNode, std::size_t... Is>
    receiver_port_vector indexer_ports(IndexerNode& dxn, std::index_sequence<Is...>)
    {
        return {dynamic_cast<receiver_type*>(&tbb::flow::input_port<Is>(dxn))...};
    }

    template<typename OutdexerNode, std::size_t... Is>
    sender_port_vector outdexer_ports(OutdexerNode& mfn, std::index_sequence<Is...>)
    {
        return {dynamic_cast<sender_type*>(&tbb::flow::output_port<Is>(mfn))...};
    }


    template<int Nin>
    using indexer_node = typename generate_indexer_node<msg_t, Nin>::type;

    template<int Nin>
    using indexer_msg_t = typename indexer_node<Nin>::output_type;

    template<int Nin, int Nout>
    using mfunc_node_type = typename generate_outdexer_node<indexer_msg_t<Nin>, msg_t, Nout>::type;

    template<int Nin, int Nout>
    using mfunc_ports_type = typename mfunc_node_type<Nin,Nout>::output_ports_type;


    template <std::size_t Nin, std::size_t Nout>
    class HydraBody {
        WireCell::IHydraNodeBase::pointer m_wcnode;

      public:
        HydraBody(WireCell::INode::pointer wcnode)
        {
            m_wcnode = std::dynamic_pointer_cast<WireCell::IHydraNodeBase>(wcnode);
        }
        void operator()(const indexer_msg_t<Nin>& in, mfunc_ports_type<Nin,Nout>& out)
        {
        }
    };

    using rxtx_port_vectors = std::pair<receiver_port_vector, sender_port_vector>;

    template <std::size_t Nin, std::size_t Nout>
    rxtx_port_vectors build_hydra(tbb::flow::graph& graph,
                                  WireCell::INode::pointer wcnode,
                                  std::vector<tbb::flow::graph_node*>& nodes)
    {

        auto* dxn = new indexer_node<Nin>(graph);
        nodes.push_back(dxn);
        receiver_port_vector recv = indexer_ports(*dxn, std::make_index_sequence<Nin>{});


        auto mfn = new mfunc_node_type<Nin, Nout>(graph, 1, HydraBody<Nin,Nout>(wcnode));
        tbb::flow::make_edge(*dxn, *mfn);

        auto spv = outdexer_ports(*mfn, std::make_index_sequence<Nout>{});

        sender_port_vector send;
        std::vector<seq_node*> seq_nodes;
        for (size_t ind=0; ind<Nout; ++ind) {
            auto qn = new seq_node(graph, [](const msg_t& m) {return m.first;});
            seq_nodes.push_back(qn);
            tbb::flow::make_edge(*spv[ind], *qn);
            send.push_back(dynamic_cast<sender_type*>(qn));
        }
        return std::make_pair(recv, send);
    }

    // Wrap TBB around WCT hydra node
    class HydraWrapper : public NodeWrapper {
        std::vector<tbb::flow::graph_node*> m_nodes;
        rxtx_port_vectors m_rxtx;
      public:
        HydraWrapper(tbb::flow::graph& graph, WireCell::INode::pointer wcnode)
        {
            const size_t nin = wcnode->input_types().size();
            const size_t nout = wcnode->output_types().size();

            Assert(nin > 0 && nin <= 3);
            Assert(nout > 0 && nout <= 3);
            if (1 == nin) {
                if (1 == nout) m_rxtx = build_hydra<1,1>(graph, wcnode, m_nodes);
                if (2 == nout) m_rxtx = build_hydra<1,2>(graph, wcnode, m_nodes);
                if (3 == nout) m_rxtx = build_hydra<1,3>(graph, wcnode, m_nodes);
            }
            if (2 == nin) {
                if (1 == nout) m_rxtx = build_hydra<2,1>(graph, wcnode, m_nodes);
                if (2 == nout) m_rxtx = build_hydra<2,2>(graph, wcnode, m_nodes);
                if (3 == nout) m_rxtx = build_hydra<2,3>(graph, wcnode, m_nodes);
            }
            if (3 == nin) {
                if (1 == nout) m_rxtx = build_hydra<3,1>(graph, wcnode, m_nodes);
                if (2 == nout) m_rxtx = build_hydra<3,2>(graph, wcnode, m_nodes);
                if (3 == nout) m_rxtx = build_hydra<3,3>(graph, wcnode, m_nodes);
            }
        }
        virtual sender_port_vector sender_ports()
        {
            return m_rxtx.second;
        }
        virtual receiver_port_vector receiver_ports()
        {
            return m_rxtx.first;
        }          
    };
}

#endif
