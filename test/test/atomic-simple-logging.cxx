#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"

int main(int argc, char* argv[])
{
    // required for SPDLOG_LEVEL env var
    spdlog::cfg::load_env_levels(); 

    spdlog::debug("all messages should be at debug or trace");
    spdlog::info("avoid use of info() despite this example");

    // Now some "tests"
    bool ok = true;
    if (!ok) return 1;

    return 0;
}
