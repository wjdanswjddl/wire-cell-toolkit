#!/usr/bin/env bats

# Test for https://github.com/WireCell/wire-cell-toolkit/issues/200
#
# Here, we pretend we build against WCT and make sure the build config
# header is found.

bats_load_library wct-bats.sh

@test "assure build config built" {
    wcb_env
    echo  "$BuildConfig" 

    [[ -n "$BuildConfig" ]]
    path="$(topdir)/build/$BuildConfig"
    echo "Expect build config file built at: $path"
    [[ -f "$path" ]]
}

@test "assure build config installed" {
    wcb_env
    [[ -n "$PREFIX" ]]

    echo "Expect build config file installed at: $PREFIX/include/$BuildConfig"
    [[ -f "$PREFIX/include/$BuildConfig" ]]
}

@test "external build with config" {
    wcb_env
    [[ -n "$CXX" ]]

    cd_tmp

    cat <<EOF > check.cxx
#include "$BuildConfig"
#include <iostream>
int main() {
    std::cout << "version " << WIRECELL_VERSION << std::endl;
    return 0;
}
EOF
    run $CXX -I $PREFIX/include -o check check.cxx
    echo $output
    [[ "$status" -eq 0 ]]

    run ./check
    echo "$output"
    [[ "$status" -eq 0 ]]

    [[ -n "$(echo $output|grep version)" ]]


}
