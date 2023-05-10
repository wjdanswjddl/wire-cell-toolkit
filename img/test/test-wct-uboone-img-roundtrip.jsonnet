local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";
local params = import "pgrapher/experiment/uboone/simparams.jsonnet";
local tools_maker = import 'pgrapher/common/tools.jsonnet';
local tools = tools_maker(params);
local anodes = tools.anodes;


function(infile="clusters-input.json", outfile="clusters-output.json", infmt="json", outfmt="json")
    local source = pg.pnode({
        type: "ClusterFileSource",
        name: "",
        data: {
            inname: infile,
            format: infmt,
            anodes: [wc.tn(a) for a in anodes],
        }
    }, nin=0, nout=1, uses=anodes);
    local sink = pg.pnode({
        type: "ClusterFileSink",
        name: "",
        data: {
            outname: outfile,
            format: outfmt,
        }
    }, nin=1, nout=0);
    local graph = pg.pipeline([source, sink]);
    local app = {
        type: 'Pgrapher',
        data: {
            edges: pg.edges(graph),
        },
    };
    local cmdline = {
        type: "wire-cell",
        data: {
            plugins: ["WireCellGen", "WireCellPgraph", "WireCellSio", "WireCellImg"],
            apps: ["Pgrapher"]
        }
    };
    [cmdline] + pg.uses(graph) + [app]


            
