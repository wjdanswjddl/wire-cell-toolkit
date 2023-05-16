#include "WireCellUtil/Stream.h"
#include <iostream>
#include <string>

using namespace WireCell::Stream;

using FArray = Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic>;
using IArray = Eigen::Array<int, Eigen::Dynamic, Eigen::Dynamic>;


int main(int argc, char** argv)

{
    if (argc < 3) {
        std::cerr << "usage: check_numpy_depos depo-file.npz\n";
    }
    const std::string fname = argv[1];

    boost::iostreams::filtering_istream si;
    input_filters(si, fname);


    FArray data;
    std::string dname="";
    read(si, dname, data);
    assert(!dname.empty());
    std::cerr << dname << " shape: (" << data.rows() << " " << data.cols() << ")\n";

    IArray info;
    std::string iname="";
    read(si, iname, info);
    assert(!iname.empty());
    std::cerr << iname << " shape: (" << info.rows() << " " << info.cols() << ")\n";
        
    assert (data.rows() == 7);
    assert (info.rows() == 4);
    
    const size_t ndepos = data.cols();
    assert(ndepos == (size_t)info.cols());

    for (size_t idepo=0; idepo<ndepos; ++idepo) {
        
        std::cerr
            << "row=" << idepo
            << " t=" << data(0, idepo)
            << " q=" << data(1, idepo)
            << " id=" << info(0, idepo)
            << " pdg=" << info(1, idepo)
            << " gen=" << info(2, idepo)
            << " child=" << info(3, idepo)
            << std::endl;
    }
    return 0;
}
