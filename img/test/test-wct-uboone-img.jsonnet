//
// This is modified version of:
// https://github.com/HaiwangYu/wcp-porting-img/blob/main/wct-celltree-img.jsonnet
//
// It is changed in these ways:
//
// 1) Produces a top-level function with all default TLAs
//
// 2) Reads a WCT tar stream file with a single event instead of the
//    "celltreeOVERLAY.root" ROOT file with 100 events.  See
//    tests/scripts/celltree-excerpt script for how to produce this
//    single event file.
// 
// 3) Subset of the ancillary img.jsonnet file at the URL above is
//    incporated directly into this file as local function.
//
// 4) Some previous options set via commenting are exposed as TLAs.
//
// 5) Some unused code is deleted.
//
// 6) Some variable renaming.
//
// 7) Outputs both a JSON and a Numpy cluster file format.


local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";
local params = import "pgrapher/experiment/uboone/simparams.jsonnet";
local tools_maker = import 'pgrapher/common/tools.jsonnet';
local tools = tools_maker(params);
local anodes = tools.anodes;

// A function that sets up slicing for an APA.
local slicing(anode, aname, tag="", span=4, active_planes=[0,1,2], masked_planes=[], dummy_planes=[]) =
    pg.pnode({
        type: "MaskSlices",
        name: "slicing-"+aname,
        data: {
            tag: tag,
            tick_span: span,
            anode: wc.tn(anode),
            min_tbin: 0,
            max_tbin: 9592,
            active_planes: active_planes,
            masked_planes: masked_planes,
            dummy_planes: dummy_planes,
        },
    }, nin=1, nout=1, uses=[anode]);

// A function sets up tiling for an APA incuding a per-face split.
local tiling(anode, aname) =
    pg.pnode({
        type: "GridTiling",
        name: "tiling-%s-face0"%aname,
        data: {
            anode: wc.tn(anode),
            face: 0,
        }
    }, nin=1, nout=1, uses=[anode]);

local cluster_sink(outfile, fmt="json") =
    pg.pnode({
        type: "ClusterFileSink",
        name: outfile,
        data: {
            outname: outfile,
            format: fmt,
        }
    }, nin=1, nout=0);

local filesource(infile, tags=[]) = pg.pnode({
    type: "FrameFileSource",
    name: infile,
    data: {
        inname: infile,
        tags: tags,
    },
}, nin=0, nout=1);

local waveform_map = {
    type: 'WaveformMap',
    name: 'wfm',
    data: {
        filename: "microboone-charge-error.json.bz2",
    },
    uses: [],
};

local charge_err = pg.pnode({
    type: 'ChargeErrorFrameEstimator',
    name: 'cefe',
    data: {
        intag: "gauss",
        outtag: 'gauss_error',
        anode: wc.tn(anodes[0]),
	rebin: 4,  // this number should be consistent with the waveform_map choice
	fudge_factors: [2.31, 2.31, 1.1],  // fudge factors for each plane [0,1,2]
	time_limits: [12, 800],  // the unit of this is in ticks
        errors: wc.tn(waveform_map),
    },
}, nin=1, nout=1, uses=[waveform_map, anodes[0]]);

local cmm_mod = pg.pnode({
    type: 'CMMModifier',
    name: '',
    data: {
        cm_tag: "bad",
        trace_tag: "gauss",
        anode: wc.tn(anodes[0]),
	start: 0,   // start veto ...
	end: 9592, // end  of veto
	ncount_cont_ch: 2,
        cont_ch_llimit: [296, 2336+4800 ], // veto if continues bad channels
	cont_ch_hlimit: [671, 2463+4800 ],
	ncount_veto_ch: 1,
	veto_ch_llimit: [3684],  // direct veto these channels
	veto_ch_hlimit: [3699],
	dead_ch_ncount: 10,
	dead_ch_charge: 1000,
	ncount_dead_ch: 2,
	dead_ch_llimit: [2160, 2080], // veto according to the charge size for dead channels
	dead_ch_hlimit: [2176, 2096],
	ncount_org: 5,   // organize the dead channel ranges according to these boundaries 
	org_llimit: [0   , 1920, 3840, 5760, 7680], // must be ordered ...
	org_hlimit: [1919, 3839, 5759, 7679, 9592], // must be ordered ...
    },
}, nin=1, nout=1, uses=[anodes[0]]);

local frame_masking = pg.pnode({
    type: 'FrameMasking',
    name: '',
    data: {
        cm_tag: "bad",
        trace_tags: ['gauss','wiener'],
        anode: wc.tn(anodes[0]),
    },
}, nin=1, nout=1, uses=[anodes[0]]);

local anode = anodes[0];

// Top-level function
function(infile="celltreeOVERLAY-event6501.tar.bz2",
         outpat="clusters-event6501-%s.tar.gz",
         formats = "json,numpy")

    local cfo = pg.pnode({
        type: "ClusterFanout",
        name: "tiled",
        data: {
            multiplicity: 2,
            tag_rules: [],
            }
    }, nin=1, nout=2);
    local dump_tiled = cluster_sink("tiled-"+outpat%"numpy", "numpy");
    local tap_tiled = pg.intern(innodes=[cfo], outnodes=[cfo], centernodes=[dump_tiled],
                                edges=[pg.edge(cfo, dump_tiled, 1, 0)]);

    local graph = pg.pipeline([
        filesource(infile, tags=["gauss","wiener"]),
        cmm_mod, // CMM modification
        frame_masking, // apply CMM
        charge_err, // calculate charge error
        slicing(anode, anode.name, "gauss"),
        tiling(anode, anode.name),
        pg.pnode({
            type: "BlobClustering",
            name: "blobclustering-" + anode.name,
            data:  { spans : 1 }
        }, nin=1, nout=1),
        pg.pnode({
            type: "BlobGrouping",
            name: "blobgrouping-" + anode.name,
            data:  {
            }
        }, nin=1, nout=1),
        tap_tiled,
        pg.pnode({
            type: "ChargeSolving",
            name: "chargesolving0-" + anode.name,
            data:  {
                weighting_strategies: ["uniform"], //"uniform", "simple"
            }
        }, nin=1, nout=1),
        pg.fan.fanout("ClusterFanout", [
            cluster_sink(outpat%fmt, fmt) for fmt in std.split(formats, ',')], "")
    ], "main");

    local app = {
        type: 'Pgrapher',
        data: {
            edges: pg.edges(graph),
        },
    };

    local cmdline = {
        type: "wire-cell",
        data: {
            plugins: ["WireCellGen", "WireCellPgraph", "WireCellSio", "WireCellSigProc", "WireCellImg"],
            apps: ["Pgrapher"]
        }
    };

    [cmdline] + pg.uses(graph) + [app]
