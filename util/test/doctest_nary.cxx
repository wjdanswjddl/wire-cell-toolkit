#include "WireCellUtil/NaryTree.h"

#include "WireCellUtil/doctest.h"

#include <string>
#include <iostream>

using namespace WireCell::NaryTree;


TEST_CASE("nary tree depth ptc iter") {
    depth_ptc_iter<int> a, b;

    CHECK( !a.node );

    CHECK( a == b );
    
    CHECK( a.begin() == a.end() );
}

// Simple node data that tracks its copies/moves
struct Data {
    std::string val{"(uninitialized)"};

    size_t ndef{0}, nctor{0}, ncopy{0}, nmove{0};

    Data() : val("") {
        ++ndef;
    }

    Data(const Data& d) {
        val = d.val;
        ndef = d.ndef;
        nctor = d.nctor;
        ncopy = d.ncopy;
        nmove = d.nmove;
        ++ncopy;
    }

    Data(Data&& d) {
        std::swap(val, d.val);
        std::swap(ndef, d.ndef);
        std::swap(nctor, d.nctor);
        std::swap(ncopy, d.ncopy);
        std::swap(nmove, d.nmove);
        ++nmove;
    }

    explicit Data(const std::string& s) : val(s) {
        ++nctor;
    }
    
};
std::ostream& operator<<(std::ostream& o, const Data& d) {
    o << d.val;
    return o;
}

TEST_CASE("nary tree node") {

    using node_type = Node<Data>;

    {
        Data d("moved");
        node_type n(std::move(d));
        CHECK(n.value.nctor == 1);
        CHECK(n.value.nmove == 1);
        CHECK(n.value.ncopy == 0);
        CHECK(n.value.ndef == 0);
    }

    {
        Data d("copied");
        node_type n(d);
        CHECK(n.value.nctor == 1);
        CHECK(n.value.nmove == 0);
        CHECK(n.value.ncopy == 1);
        CHECK(n.value.ndef == 0);
    }
    node_type r(Data("0"));

    {
        Data d("0.0");
        r.insert(d);
    }
    {
        Data d("0.1");
        r.insert(std::move(d));
    }
    {
        r.insert(std::make_unique<node_type>(Data("0.2")));
    }

    const auto& childs = r.children();
    auto chit = childs.begin();

    {
        CHECK ( (*chit)->parent == &r );

        auto& d = (*chit)->value;

        CHECK( d.val == "0.0" );
        CHECK( d.nctor == 1);
        CHECK( d.nmove == 0);
        CHECK( d.ncopy == 1);
        CHECK( d.ndef == 0);

        ++chit;
    }
    {
        CHECK ( (*chit)->parent == &r );

        auto& d = (*chit)->value;

        CHECK( d.val == "0.1" );
        CHECK( d.nctor == 1);
        CHECK( d.nmove == 1);
        CHECK( d.ncopy == 0);
        CHECK( d.ndef == 0);

        ++chit;
    }
    {
        CHECK ( (*chit)->parent == &r );

        auto& d = (*chit)->value;

        CHECK( d.val == "0.2" );
        CHECK( d.nctor == 1);
        CHECK( d.nmove == 1);
        CHECK( d.ncopy == 0);
        CHECK( d.ndef == 0);

        ++chit;
    }

    CHECK(chit == childs.end());

    CHECK( childs.front()->next()->value.val == "0.1" );
    CHECK( childs.back()->prev()->value.val == "0.1" );

    {
        size_t nnodes = 0;
        depth_ptc_iter<Data> depth(&r);
        std::vector<Data> data;
        for (auto it = depth.begin();
             it != depth.end();
             ++it)
        {
            const Data& d = it.node->value;
            std::cerr << "depth " << nnodes << " " << d << "\n";
            ++nnodes;
            data.push_back(d);
        }
        CHECK( nnodes == 4 );

        CHECK( data[0].val == "0" );
        CHECK( data[1].val == "0.0" );
        CHECK( data[2].val == "0.1" );
        CHECK( data[3].val == "0.2" );

    }

}    
