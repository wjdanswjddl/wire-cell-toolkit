load ../wct-bats.sh

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
    [[ "$status" -eq 0 ]]
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

@test "have a test data file or skip" {
    local name="pdsp/sim/sn/depos.tar.bz2"

    skip_if_no_test_data "$name"

    [[ -n "$(test_data_dir)" ]]
    [[ -d "$(test_data_dir)" ]]

    [[ -n "$(test_data_file $name)" ]]
    [[ -f "$(test_data_file $name)" ]]
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
    
