#include "WireCellUtil/Rectangles.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace WireCell;

int main(int argc, char* argv[])
{
    using rec_t = Rectangles<char, int>;
    rec_t recs;

    recs.add(0,7, 0, 4, 'a');
    recs += rec_t::rectangle_t(rec_t::xinterval_t(3,11),
                               rec_t::yinterval_t(2, 6), 'b');

    std::string fname = argv[0];
    fname += ".euk";
    std::ofstream fstr(fname);

    fstr << "frame -1, -1, 12, 7\n";

    std::vector<std::string> colors = {
        "red", "green", "blue", "cyan", "magenta"
    };

    int ind=0;
    for (const auto& [xi, yi, cs] : recs.regions()) {
        rect_t::xkey_t x1 = xi.lower();
        rect_t::xkey_t x2 = xi.upper();
        rect_t::ykey_t y1 = yi.lower();
        rect_t::ykey_t y2 = yi.upper();

        fstr << "draw ["
             << "point(" << x1 << "," << y1 << ")."
             << "point(" << x1 << "," << y2 << ")."
             << "point(" << x2 << "," << y2 << ")."
             << "point(" << x2 << "," << y1 << ")."
             << "point(" << x1 << "," << y1 << ")] "
             << colors[ind++] << "\n";
    }
    std::cerr << "eukleides --output=" << argv[0] << ".pdf " << fname << "\n";
    return 0;
}
