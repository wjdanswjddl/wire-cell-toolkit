#!/usr/bin/env bats

load "../../test/wct-bats.sh"

setup_file () {
    [[ "$(divine_context)" == "file" ]]
}

@test "devine context" {
    [[ "$(divine_context)" == "test" ]]
}

@test "test wct bats" {
    usepkg util apps cfg
    t=$(topdir)
    [ -f "$t/build/apps/wire-cell" ] 
    [ -n "$util_src" ]
    [ -d "$util_src" ]
    [ -n "$apps_src" ]
    [ -d "$apps_src" ]
    [ -n "$cfg_src" ]
    [ -d "$cfg_src" ]
}

# tip: give bats the argument --show-output-of-passing-tests  
@test "dump wcb env" {
    run wcb dumpenv
    echo "$output" |grep '^wcb: '| sed -e 's/wcb://'
    [[ "$status" -eq 0 ]]
}
@test "dump shell env" {
    run bash -c "env|grep =|sort"
    echo "$output"
    [[ "$status" -eq 0 ]]
}
@test "dump bats env" {
    run bash -c "env|grep BATS|sort"
    echo "$output"
    [[ "$status" -eq 0 ]]
}
    
@test "saveout" {
    local odir="$(blddir)/tests/output/$(version)/$(basename $BATS_TEST_FILENAME .bats)"
    local jdir="$(blddir)/tests/junk/$(version)/$(basename $BATS_TEST_FILENAME .bats)"

    # this writes to user's cwd
    echo "saveout test file outside of tmp" > junk.txt
    saveout junk.txt
    rm junk.txt
    [[ -f "$odir/junk.txt" ]]
    
    cd_tmp
    echo "saveout test file 1" > junk1.txt
    echo "saveout test file 2" > junk2.txt

    saveout junk1.txt 
    [[ -f "$odir/junk1.txt" ]]

    saveout -c junk junk1.txt junk2.txt
    [[ -f "$jdir/junk1.txt" ]]
    [[ -f "$jdir/junk2.txt" ]]

    saveout junk2.txt junk3.txt
    [[ -f "$odir/junk3.txt" ]]

    cd_tmp file
    echo "file in file tmp context" >  junk4.txt
    saveout junk4.txt
    [[ -f "$odir/junk4.txt" ]]    
}

@test "divine context" {
    local got=$(divine_context)
    if [[ "$got" != "test" ]] ; then
        die "divine context fails, want test got $got"
    fi
}
