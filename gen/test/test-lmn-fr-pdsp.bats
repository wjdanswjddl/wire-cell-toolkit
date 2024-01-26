#!/usr/bin/env bats

# Run things for LMN FR resampling test

bats_load_library wct-bats.sh

@test "lmn fr preplots" {
    cd_tmp file
    wirecell-resp lmn-fr-plots  \
                  --period '100*ns' --zoom-start '50*us' \
                  --plane 0 \
                  --detector-name pdsp \
                  -o "lmn-fr-pdsp-0.pdf" \
                  -O "lmn-fr-pdsp-0.org"
}

fields1="pdsp-fields-100ns.json.bz2"
fields2="pdsp-fields-64ns.json.bz2"

@test "lmn fr resample fr" {
    cd_tmp file
    local name="pdsp"
    orig="$(wirecell-util resolve -k fields "$name" | head -1)"


    # default pdsp fr.period is not exactly 100ns
    wirecell-resp condition -P '100*ns' -o "$fields1" "$orig"
    wirecell-resp resample  -t  '64*ns' -o "$fields2" "$fields1"
}

@test "lmn fr gen depos" {
    wirecell-gen depo-line \
                 --electron-density '-5000' \
                 --first '-3578.36*mm,76.1*mm,3.3497*mm' \
                 --last '-1.5875*mm,6060*mm,2303.37*mm' \
                 --sigma '1*mm,1*mm' \
                 --output depos.npz
}

@test "lmn fr sim" {
    local name=pdsp
    for tick in 500 512
    do
        log="wct-${name}-${tick}ns.log"
        rm -f "$log"
        wire-cell -l "$log" -L debug \
                  -A input="depos.npz" \
                  --tla-code output="{sim:\"digits-${tick}ns.npz\"}" \
                  -A detector="${name}" \
                  -A variant="resample_${tick}ns" \
                  -A tasks="drift,sim" \
                  layers/omnijob.jsonnet
    done
}

@test "lmn fr resample digits" {
    # "null resampling" merely change name to follow downstream pattern.
    ln -sf digits-500ns.npz digits-500ns-500ns.npz

    # note, eventually this should be done inside NF in order to save one FFT
    # round trip per channel.
    wirecell-util resample -t '500*ns' -o digits-512ns-500ns.npz digits-512ns.npz
}

@test "lmn fr sigproc" {
    local name=pdsp
    for tick in 500 512
    do
        wire-cell -l wct.log -L debug \
                  -A input="digits-${tick}ns-500ns.npz" \
                  --tla-code output="{sp:\"signals-${tick}ns-500ns.npz\"}" \
                  -A detector="${name}" \
                  -A variant="resample_500ns" \
                  -A tasks="nf,sp" \
                  layers/omnijob.jsonnet
    done

}
