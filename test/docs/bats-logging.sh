set -x
bats -f "logging" test/test/test_wct_bats.bats

WCT_BATS_LOG_SINK=terminal bats -f "logging" test/test/test_wct_bats.bats
WCT_BATS_LOG_SINK=terminal WCT_BATS_LOG_LEVEL=error bats -f "logging" test/test/test_wct_bats.bats

rm -f junk.log
WCT_BATS_LOG_SINK=junk.log bats -f "logging" test/test/test_wct_bats.bats
cat junk.log

