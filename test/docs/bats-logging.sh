set -x

# Defaults: "info" level, output only on failure.
bats -f "logging" test/test/test_wct_bats.bats

# All output greater than or equal to default "info" to terminal.
WCT_BATS_LOG_SINK=terminal bats -f "logging" test/test/test_wct_bats.bats

# All output greater than or equal to "error" to terminal.
WCT_BATS_LOG_SINK=terminal WCT_BATS_LOG_LEVEL=error bats -f "logging" test/test/test_wct_bats.bats

# All output greater than or equal to "info" to file.
rm -f junk.log
WCT_BATS_LOG_SINK=junk.log bats -f "logging" test/test/test_wct_bats.bats
cat junk.log

