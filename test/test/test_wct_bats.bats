bats_load_library wct-bats.sh

@test "check usepkg" {

    usepkg cfg
    [[ -n "$cfg_src" ]]
    [[ -d "$cfg_src" ]]

    cd_tmp
    usepkg util
    [[ -n "$util_src" ]]
    [[ -d "$util_src" ]]
}

@test "resolve file in src" {
    local got="$(resolve_file test/test/test_wct_bats.bats)"
    [[ -n "$got" ]]
    [[ -n "$BATS_TEST_FILENAME" ]]
    a="$(cat $BATS_TEST_FILENAME | md5sum)"
    b="$(cat $got | md5sum)"
    echo "$a"
    echo "$b"
    [[ "$a" = "$b" ]]
}

@test "tojson" {
    run tojson foo=42 bar="baz=quax quax"
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -n "$(echo $output | grep '"foo": "42"')" ]]
    [[ -n "$(echo $output | grep '"bar": "baz=quax quax"')" ]]
}

@test "wcb env" {
    [[ -z "$PREFIX" ]]
    local prefix=$(wcb_env_value PREFIX)
    [[ -z "$(echo $prefix | grep '"')" ]]
    [[ -z "$PREFIX" ]]

    # only one var
    run wcb_env_vars VERSION
    echo $output
    [[ "$status" -eq 0 ]]
    [[ -n "$(echo $output | grep VERSION)" ]]
    [[ -z "$(echo $output | grep SUBDIRS)" ]]

    # all vars
    run wcb_env_vars 
    echo $output
    [[ "$status" -eq 0 ]]
    [[ -n "$(echo $output | grep VERSION)" ]]
    [[ -n "$(echo $output | grep SUBDIRS)" ]]

    # specific environment 
    [[ -z "$VERSION" ]]
    wcb_env VERSION
    [[ -n "$VERSION" ]]
    [[ -z "$SUBDIRS" ]]

    # everything
    wcb_env
    [[ -n "$VERSION" ]]
    [[ -n "$SUBDIRS" ]]

}

@test "wcb env in temp dir" {
    cd_tmp
    [[ -z "$PREFIX" ]]
    local prefix=$(wcb_env_value PREFIX)
    [[ -z "$(echo $prefix | grep '"')" ]]
    [[ -z "$PREFIX" ]]

    # note to any users reading this test for examples, don't use
    # wcsonnet directly but instead call compile_jsonnet.
    wcsonnet=$(wcb_env_value WCSONNET)
    echo "wcsonnet=|$wcsonnet|"
    [[ -n "$wcsonnet" ]]
}


@test "have a test data file or skip" {
    local name="depos/many.tar.bz2"

    skip_if_no_input "$name"

    local got=$(input_file $name)
    [[ -s $got ]]
}

@test "relative path" {
    local got="$(relative_path check_batsio.bats)"
    echo "$got"
    [[ -n "$got" ]]
    [[ -f "$got" ]]
    [[ -n "$(echo $got | grep '^/')" ]]
}

@test "tmp dirs" {
    if [ -n "$WCTEST_TMPDIR" ] ; then
        [[ "$(tmpdir)" = "$WCTEST_TMPDIR" ]]
        return
    fi

    [[ -n "$BATS_RUN_TMPDIR" ]]
    [[ -n "$BATS_FILE_TMPDIR" ]]
    [[ -n "$BATS_TEST_TMPDIR" ]]
    [[ -n "$BATS_TEST_TMPDIR" ]]

    [[ -n "$(tmpdir run)" ]]
    [[ -n "$(tmpdir file)" ]]
    [[ -n "$(tmpdir test)" ]]
    [[ -n "$(tmpdir giraffea)" ]]

    [[ "$(tmpdir run)" = "$BATS_RUN_TMPDIR" ]]
    [[ "$(tmpdir file)" = "$BATS_FILE_TMPDIR" ]]
    [[ "$(tmpdir test)" = "$BATS_TEST_TMPDIR" ]]
    [[ "$(tmpdir giraffea)" = "$BATS_TEST_TMPDIR" ]]

    cd_tmp 
    [[ "$(pwd)" = "$BATS_TEST_TMPDIR" ]]
    cd_tmp file
    [[ "$(pwd)" = "$BATS_FILE_TMPDIR" ]]
}


@test "bats run command env var" {
    run env
    echo "run command: $BATS_RUN_COMMAND"
    [[ -n "$BATS_RUN_COMMAND" ]]
    # why is it not in env?
    [[ -z "$(env |grep BATS_RUN_COMMAND)" ]]
    export BATS_RUN_COMMAND
    [[ -n "$(env |grep BATS_RUN_COMMAND)" ]]
}
    
@test "pathlist resolution" {
    local t=$(topdir)
    declare -a got=( $(resolve_pathlist $t/gen:$t/test $t/sigproc:$t/apps $t/doesnotexist) )
    # yell ${got[*]}
    [[ ${#got[@]} -eq 4 ]]
    [[ ${got[0]} = $t/gen ]]
    [[ ${got[1]} = $t/test ]]
    [[ ${got[2]} = $t/sigproc ]]
    [[ ${got[3]} = $t/apps ]]
}

@test "category resolution" {
    
    local category="testing"
    local test_data="$(blddir)/tests"
    local release=0.0.0
    local dirty="${release}-dirty"

    for one in $release $dirty
    do
        dir=$test_data/$category/$one
        rm -rf $dir
        mkdir -p $dir
        touch $dir/junk.txt
    done

    declare -a got=( $(find_category_version_paths -c testing) )
    [[ ${#got[@]} -eq 1 ]]
    [[ ${got[0]} = "$test_data/testing/$release" ]]

    got=( $(find_category_version_paths -c testing -d) )
    [[ ${#got[@]} -eq 2 ]]
    [[ ${got[0]} = "$test_data/testing/$release" ]]
    [[ ${got[1]} = "$test_data/testing/$dirty" ]]

    got=( $(find_category_version_paths -c testing -d) )
    [[ ${#got[@]} -eq 2 ]]
    [[ ${got[0]} = "$test_data/testing/$release" ]]
    [[ ${got[1]} = "$test_data/testing/$dirty" ]]
    

    declare -a got3=( $(find_paths junk.txt ${got[*]}) )
    [[ ${#got3[@]} -eq 2 ]]
    [[ "$(basename ${got3[0]})" = "junk.txt" ]]
    [[ "$(basename ${got3[1]})" = "junk.txt" ]]


    vers=( $(for n in ${got[*]}; do basename $n; done | sort -u | tail -1 ) )
    [[ "$vers" = "0.0.0-dirty" ]]
    

}
