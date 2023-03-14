load ../wct-bats.sh

@test "resolve file in src" {
#    env|grep =|sort
    top
    local got="$(resolve_file test/test/test_wct_bats.bats)"
    [[ -n "$got" ]]
    a="$(cat $BATS_TEST_FILENAME | md5sum)"
    b="$(cat $got | md5sum)"
    echo "$a"
    echo "$b"
    [[ "$a" = "$b" ]]
}

@test "divine wire cell test data path" {
    if [ -n "$WIRECELL_TEST_DATA_PATH" ] ; then
        unset WIRECELL_TEST_DATA_PATH
    fi
    assure_wirecell_test_data
    echo "$WIRECELL_TEST_DATA_PATH"
    [[ -n "$WIRECELL_TEST_DATA_PATH" ]]
}
@test "retain wire cell test data path" {
    if [ -z "$WIRECELL_TEST_DATA_PATH" ] ; then
        WIRECELL_TEST_DATA_PATH="/tmp"
    fi
    local old="$WIRECELL_TEST_DATA_PATH" 
    assure_wirecell_test_data
    echo "$WIRECELL_TEST_DATA_PATH"
    [[ "$WIRECELL_TEST_DATA_PATH" = "$old" ]]
}

# @test "blah" {
#     dumpenv > /tmp/batsenv.dump
#     exit 1
# }
    
