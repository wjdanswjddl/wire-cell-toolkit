#define DOCTEST_CONFIG_IMPLEMENT
#include "WireCellUtil/doctest.h"
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"
int main(int argc, char** argv) {
    spdlog::cfg::load_env_levels();
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int rc = context.run();
    if (rc) { return rc; }

    bool ok = true;             // start my tests
    if (!ok) return 1;
    return 0;
}


