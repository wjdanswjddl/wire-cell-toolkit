// This converts a celltree file to a WCT frame file

local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";
local all_params = {
    uboone: import "pgrapher/experiment/uboone/simparams.jsonnet",
    pdsp: import "pgrapher/experiment/pdsp/simparams.jsonnet",
};    
local hs = import "pgrapher/common/helpers.jsonnet";

function(infile="celltree.root", outfile="clusters.tar.bz2", eventid="6501", det="uboone") {
    local params = all_params[det],
    local wires = hs.aux.wires(params.files.wires),
    local anodes = hs.aux.anodes(wires, params.det.volumes),
    local tags = ["gauss"],
    local digitize = false,

    local source = pg.pnode({
        type: "CelltreeSource",
        data: {
            filename: infile,
            EventNo: eventid,   // note, must NOT be a number but a string....
            frames: tags,
            in_branch_base_names: ["calibWiener", "calibGaussian"],
            out_trace_tags: ["wiener", "gauss"], // orig, gauss, wiener
        },
    }, nin=0, nout=1),

    local imgpipe(anode) = pg.pipeline([
        hs.img(anode).simple(),
        hs.io.cluster_file_sink(anode.data.ident,
                           "test-clusters-%s-%d.tar.bz2"%[eventid, anode.data.ident])
    ], "img-" + anode.name),

    // fixme: make this work with multi anode detectors
    local graph = pg.pipeline([source, imgpipe(anodes[0])], "main"),
    local app = {
        type: 'Pgrapher',
        data: {
            edges: pg.edges(graph),
        },
    },

    local cmdline = {
        type: "wire-cell",
        data: {
            plugins: ["WireCellGen", "WireCellPgraph", "WireCellSio", "WireCellSigProc", "WireCellRoot", "WireCellImg"],
            apps: ["Pgrapher"]
        }
    },

    res: [cmdline] + pg.uses(graph) + [app],
}.res


        
