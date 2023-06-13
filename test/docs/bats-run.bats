# bats-run.test
@test "always fails" {
    echo "this still always fails"
    run false
    echo "we ran the failing job and it produced: \"$output\" which happens to be empty"
    [[ $status -eq 0 ]]
}
