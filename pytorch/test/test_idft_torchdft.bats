#!/usr/bin/env bats

bats_load_library wct-bats.sh

@test "test_idft with torchdft" {
    usepkg aux                  # to get test_idft
    check test_idft TorchDFT WireCellPytorch
}
