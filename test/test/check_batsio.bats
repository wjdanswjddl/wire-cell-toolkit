# See wscript_build for how this VARIANT test is registered.  That
# registration must collude with the test code here so that the same
# env var names are used to communicate input and output files.  We
# use "WCTEST_*" prefixes in the variable names but any variable will
# do.
@test "assure we get wscript_build" {

    echo "input: $WCTEST_INPUT"
    echo "output: $WCTEST_OUTPUT"

    [[ -n "$WCTEST_INPUT" ]]
    [[ -f "$WCTEST_INPUT" ]]
    [[ -n "$WCTEST_OUTPUT" ]]
    
    [[ -n "$(grep WCTEST_INPUT ${WCTEST_INPUT})" ]] 
    [[ -n "$(grep WCTEST_OUTPUT ${WCTEST_INPUT})" ]] 

    # satisfy output
    cp "$WCTEST_INPUT" "$WCTEST_OUTPUT"
}
