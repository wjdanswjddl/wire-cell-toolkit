/*
  Generate microboone wires using numbers gleended from microboone
  larsoft wires dump.

  wirecell-util wire-summary ~/opt/wire-cell-data/microboone-celltree-wires-v2.1.json.bz2 | jq '.[].anodes[].faces[].plan
es[].pray'
  wirecell-util wire-summary ~/opt/wire-cell-data/microboone-celltree-wires-v2.1.json.bz2 | jq '.[].anodes[].faces[].plan
es[].bb'

 */

#include <WireCellUtil/WireSchema.h>
#include <vector>
#include <iostream>

using namespace WireCell;
using namespace WireCell::WireSchema;

int main(int argc, char* argv[]) {

    std::vector<Ray> boundes = {
        Ray(Point(-6.34915e-13, -1155.1000000000001, 0.35260800000000003),
            Point(6.34915e-13, 1174.5, 10369.6)),
        Ray(Point(-3, -1155.1000000000001, 0.35260800000000003),
            Point(-3, 1174.5, 10369.6)),
        Ray(Point(-6, -1155.3, 2.5),
            Point(-6, 1174.7, 10367.5))
    };
    std::vector<Ray> pitches = {
        Ray(Point(-6.34601e-13, 1173.0149999999999, 2.919594),
            Point(-6.34601e-13, 1170.4147600126691, 4.420849440518711)),
        Ray(Point(-3, -1153.615, 2.919594),
            Point(-3, -1151.0147600126693, 4.420849440518712)),
        Ray(Point(-6, 9.700000000000045, 2.5),
            Point(-6, 9.700000000000045, 5.501737116386797))
    };

    std::vector<int> expected_nwires = {2400, 2400, 3456};

    StoreDB storedb;
    for (int iplane=0; iplane<3; ++iplane) {
        auto& plane = get_append(storedb, iplane, 0, 0, 0);
        
        const Ray& bounds = boundes[iplane];
        const Ray& pitch = pitches[iplane];
        int nwires = generate(storedb, plane, pitch, bounds);
        std::cerr << iplane << " " << nwires << " =?= " << expected_nwires[iplane] << "\n";
    }

    Store store(std::make_shared<StoreDB>(storedb));
    try {
        validate(store);
    }
    catch (ValueError& err) {
        std::cerr << "invalid\n";
        return -1;
    }
    std::cerr << "valid\n";

    std::string fname = argv[0];
    fname += ".json.bz2";
    dump(fname.c_str(), store);
    
}
