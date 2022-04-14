// This converts a celltree file to a WCT frame file

local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";
local all_params = {
    uboone: import "pgrapher/experiment/uboone/simparams.jsonnet",
    pdsp: import "pgrapher/experiment/pdsp/simparams.jsonnet",
};    
local hs = import "pgrapher/common/helpers.jsonnet";

function(infile="celltree.root", outfile="frames.tar.bz2", eventid="6501", det="uboone") {
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

    local target = pg.pnode({
            type: "FrameFileSink",
            data: {
                outname: outfile,
                tags: tags,
                digitize: digitize,
            },
        }, nin=1, nout=0),
        
    local graph = pg.pipeline([source, target]),

    local app = {
        type: 'Pgrapher',
        data: {
            edges: pg.edges(graph),
        },
    },

    local cmdline = {
        type: "wire-cell",
        data: {
            plugins: ["WireCellPgraph", "WireCellSio", "WireCellRoot"],
            apps: [wc.tn(app)]
        }
    },
        
    res: [cmdline] + pg.uses(graph) + [app],
}.res


        
