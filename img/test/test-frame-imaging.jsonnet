// This is a main wire-cell config.
//
// This sources a "frame file" (tar stream of numpy arrays), runs
// imaging and writes a "cluster file".
//
// Example command:
// 
// wire-cell -l stdout -L trace -L pgraph:debug \
//           -A detector=pdsp \
//           -A infile=frames.tar.bz2 \
//           -A outfile=clusters.tar.bz2 \
//           -c test-frame-imaging.jsonnet
// 

local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";
local hs = import "pgrapher/common/helpers.jsonnet";
local plu = import "pgrapher/experiment/params.jsonnet";

local imgpipe(anode, outfile, format="json") = {
    local img = hs.img(anode),  // anode sets the names
    local tag="",
    local span=4,
    local spans=1.0,

    ret: pg.pipeline([
        img.slicing(tag=tag, span=span),
        img.tiling(),
        img.clustering(spans=spans),
        img.grouping(),
        img.charge_solving(whiten=false),
        hs.io.cluster_file_sink(anode.data.ident, outfile, format=format),
    ], "img-" + anode.name),
}.ret;


function(infile, tags=["gauss"], outfile="clusters.tar.bz2", detector='uboone', ptype='params', format='json') {
    local params = plu(detector, ptype),

    local wires = hs.aux.wires(params.files.wires),
    local anodes = hs.aux.anodes(wires, params.det.volumes),

    local src = hs.io.frame_file_source(infile, tags),

    // need a source! and need to handle multi-anodes!
    local graph = pg.pipeline([src, imgpipe(anodes[0], outfile, format)], "main"),

    ret: hs.utils.main(graph)
}.ret
