local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";
local params = import "pgrapher/experiment/uboone/simparams.jsonnet";
local tools_maker = import 'pgrapher/common/tools.jsonnet';
local tools = tools_maker(params);
local anodes = tools.anodes;


function(infile="clusters-input.json", outfile="clusters-output.json", infmt="json", outfmt="json")
    local tfsource = pg.pipeline([
        pg.pnode({
            type:"TensorFileSource",
            name: "",
            data: {
                inname: infile,
                prefix: "cluster_"
            }
        }, nin=0, nout=1),
        pg.pnode({
            type:"TensorCluster",
            name: "",
            data: {
                dpre: "clusters/[[:digit:]]+", // match ClusterTensor.datapath
                anodes: [wc.tn(a) for a in anodes],
            }
        }, nin=1, nout=1, uses=anodes)]);
    local cfsource = 
        pg.pnode({
            type: "ClusterFileSource",
            name: "",
            data: {
                inname: infile,
                format: infmt,
                anodes: [wc.tn(a) for a in anodes],
            }
        }, nin=0, nout=1, uses=anodes);

    local source = if infmt == "tensor" then tfsource else cfsource;

    local tfsink = pg.pipeline([
        pg.pnode({
            type:"ClusterTensor",
            name:"",
            data: {
                datapath: "clusters/%d" // match TensorCluster.dpre
            },
        }, nin=1, nout=1),
        pg.pnode({
            type:"TensorFileSink",
            name:"",
            data: {
                outname: outfile,
                prefix: "cluster_",
            },
        }, nin=1, nout=0)]);

    local cfsink =
        pg.pnode({
            type: "ClusterFileSink",
            name: "",
            data: {
                outname: outfile,
                format: outfmt,
            }
        }, nin=1, nout=0);

    local sink = if outfmt == "tensor" then tfsink else cfsink;

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


            
