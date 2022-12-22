// A wire cell CLI to operate on description of wires

#include "CLI11.hpp"

#include "WireCellUtil/WireSchema.h"

#include <iostream>
#include <vector>
#include <map>

using namespace WireCell;
using namespace WireCell::WireSchema;

static void
parse_param(std::string name,
            const std::vector<std::string>& args,
            std::map<std::string,std::string>& store)
{
    for (auto one : args) {
        auto two = String::split(one, "=");
        if (two.size() != 2) {
            std::cerr
                << name
                << ": parameters are set as <name>=<value>, got "
                << one << std::endl;
            throw CLI::CallForHelp();
        }
        store[two[0]] = two[1];
    }
}

int main(int argc, char** argv)
{
    CLI::App app{"wcwires converts and validates Wire-Cell Toolkit wire descriptions"};

    std::string filename="", output="";
    std::vector<std::string> load_path;
    int correction = 3;
    bool validate = false;

    app.add_option("-P,--path", load_path,
                   "Search paths to consider in addition to those in WIRECELL_PATH"
        )->type_size(1)->allow_extra_args(false);
    app.add_option("-o,--output", output,
                   "Write out a wires file (def=none)"
        )->type_size(1)->allow_extra_args(false);
    app.add_option("-c,--correction", correction,
                   "Correction level: 0,1,2,3 (def=3)"
        )->type_size(1)->allow_extra_args(false);
    app.add_flag("-v,--validate", validate,
                 "Perform input validation (def=false)"
        )->type_size(1)->allow_extra_args(false);
    app.add_option("file", filename, "wires file");
                    
    // app.set_help_flag();
    // auto help = app.add_flag("-h,--help", "Print help message");

    CLI11_PARSE(app, argc, argv);

    if (validate) {
        Store raw = load(filename.c_str(), Correction::load);
        raw.validate();
    }

    if (! output.empty()) {
        Store store = load(filename.c_str(), (Correction)correction);
        dump(output.c_str(), store);
    }
    return 0;
}
