//
// This is modified version of:
// https://github.com/HaiwangYu/wcp-porting-img/blob/main/wct-celltree-img.jsonnet
//
// It is changed in these ways:
//
// 1) Produces a top-level function with all default TLAs
//
// 2) Reads a WCT tar stream file with a single event instead of the
// "celltreeOVERLAY.root" ROOT file with 100 events.
//
// The default WCT tar stream file used was produced with:
//
// wire-cell -A infile=celltreeOVERLAY.root \
//           -A outfile=celltreeOVERLAY-event6501.tar.bz2 \
//           root/test/test-celltree-to-framefile.jsonnet
//
// This file is provided by the wire-cell-test-data repo.
//
// 3) The ancillary img.jsonnet file at the URL above is incporated
// directly into this file.
//
// 4) Some previous options-set-via-commenting are exposed as TLAs.
//
// 5) Some unused code is deleted.
//
// 6) Slight variable rename.


local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";
local params = import "pgrapher/experiment/uboone/simparams.jsonnet";
local tools_maker = import 'pgrapher/common/tools.jsonnet';
local tools = tools_maker(params);
local anodes = tools.anodes;

local img = {
    // A functio that sets up slicing for an APA.
    slicing :: function(anode, aname, tag="", span=4, active_planes=[0,1,2], masked_planes=[], dummy_planes=[]) {
        ret: pg.pnode({
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
        }, nin=1, nout=1, uses=[anode]),
    }.ret,

    // A function sets up tiling for an APA incuding a per-face split.
    tiling :: function(anode, aname) {

        local slice_fanout = pg.pnode({
            type: "SliceFanout",
            name: "slicefanout-" + aname,
            data: { multiplicity: 2 },
        }, nin=1, nout=2),

        local tilings = [pg.pnode({
            type: "GridTiling",
            name: "tiling-%s-face%d"%[aname, face],
            data: {
                anode: wc.tn(anode),
                face: face,
            }
        }, nin=1, nout=1, uses=[anode]) for face in [0,1]],

        local blobsync = pg.pnode({
            type: "BlobSetSync",
            name: "blobsetsync-" + aname,
            data: { multiplicity: 2 }
        }, nin=2, nout=1),

        // ret: pg.intern(
        //     innodes=[slice_fanout],
        //     outnodes=[blobsync],
        //     centernodes=tilings,
        //     edges=
        //         [pg.edge(slice_fanout, tilings[n], n, 0) for n in [0,1]] +
        //         [pg.edge(tilings[n], blobsync, 0, n) for n in [0,1]],
        //     name='tiling-' + aname),
        ret : tilings[0],
    }.ret,

    //
    multi_active_slicing_tiling :: function(anode, name, tag="gauss", span=4) {
        local active_planes = [[0,1,2],[0,1],[1,2],[0,2],],
        local masked_planes = [[],[2],[0],[1]],
        local iota = std.range(0,std.length(active_planes)-1),
        local slicings = [$.slicing(anode, name+"_%d"%n, tag, span, active_planes[n], masked_planes[n]) 
                          for n in iota],
        local tilings = [$.tiling(anode, name+"_%d"%n)
                         for n in iota],
        local multipass = [pg.pipeline([slicings[n],tilings[n]]) for n in iota],
        ret: pg.fan.pipe("FrameFanout", multipass, "BlobSetMerge", "multi_active_slicing_tiling"),
    }.ret,

    //
    multi_masked_2view_slicing_tiling :: function(anode, name, tag="gauss", span=109) {
        local dummy_planes = [[2],[0],[1]],
        local masked_planes = [[0,1],[1,2],[0,2]],
        local iota = std.range(0,std.length(dummy_planes)-1),
        local slicings = [$.slicing(anode, name+"_%d"%n, tag, span,
                                    active_planes=[],masked_planes=masked_planes[n], dummy_planes=dummy_planes[n])
                          for n in iota],
        local tilings = [$.tiling(anode, name+"_%d"%n)
                         for n in iota],
        local multipass = [pg.pipeline([slicings[n],tilings[n]]) for n in iota],
        ret: pg.fan.pipe("FrameFanout", multipass, "BlobSetMerge", "multi_masked_slicing_tiling"),
    }.ret,
    //
    multi_masked_slicing_tiling :: function(anode, name, tag="gauss", span=109) {
        local active_planes = [[2],[0],[1],[]],
        local masked_planes = [[0,1],[1,2],[0,2],[0,1,2]],
        local iota = std.range(0,std.length(active_planes)-1),
        local slicings = [$.slicing(anode, name+"_%d"%n, tag, span, active_planes[n], masked_planes[n]) 
                          for n in iota],
        local tilings = [$.tiling(anode, name+"_%d"%n)
                         for n in iota],
        local multipass = [pg.pipeline([slicings[n],tilings[n]]) for n in iota],
        ret: pg.fan.pipe("FrameFanout", multipass, "BlobSetMerge", "multi_masked_slicing_tiling"),
    }.ret,

    // Just clustering
    clustering :: function(anode, aname, spans=1.0) {
        ret : pg.pnode({
            type: "BlobClustering",
            name: "blobclustering-" + aname,
            data:  { spans : spans }
        }, nin=1, nout=1),
    }.ret, 

    // this bundles clustering, grouping and solving.  Other patterns
    // should be explored.  Note, anode isn't really needed, we just
    // use it for its ident and to keep similar calling pattern to
    // above..
    solving :: function(anode, aname, spans=1.0, threshold=0.0) {
        local bc = pg.pnode({
            type: "BlobClustering",
            name: "blobclustering-" + aname,
            data:  { spans : spans }
        }, nin=1, nout=1),
        local bg = pg.pnode({
            type: "BlobGrouping",
            name: "blobgrouping-" + aname,
            data:  {
            }
        }, nin=1, nout=1),
        local bs = pg.pnode({
            type: "BlobSolving",
            name: "blobsolving-" + aname,
            data:  { threshold: threshold }
        }, nin=1, nout=1),
        local cs0 = pg.pnode({
            type: "ChargeSolving",
            name: "chargesolving0-" + aname,
            data:  {
                weighting_strategies: ["uniform"], //"uniform", "simple"
            }
        }, nin=1, nout=1),
        local cs1 = pg.pnode({
            type: "ChargeSolving",
            name: "chargesolving1-" + aname,
            data:  {
                weighting_strategies: ["uniform"], //"uniform", "simple"
            }
        }, nin=1, nout=1),
        local lcbr = pg.pnode({
            type: "LCBlobRemoval",
            name: "lcblobremoval-" + aname,
            data:  {
                blob_value_threshold: 1e6,
                blob_error_threshold: 0,
            }
        }, nin=1, nout=1),
        local cs = pg.intern(
            innodes=[cs0], outnodes=[cs1], centernodes=[],
            edges=[pg.edge(cs0,cs1)],
            name="chargesolving-" + aname),
        local csp = pg.intern(
            innodes=[cs0], outnodes=[cs1], centernodes=[lcbr],
            edges=[pg.edge(cs0,lcbr), pg.edge(lcbr,cs1)],
            name="chargesolving-" + aname),
        local solver = cs0,
        ret: pg.intern(
            innodes=[bc], outnodes=[solver], centernodes=[bg],
            edges=[pg.edge(bc,bg), pg.edge(bg,solver)],
            name="solving-" + aname),
        // ret: bc,
    }.ret,

    dump(outfile) :: {

        local cs = pg.pnode({
            type: "ClusterFileSink",
            name: outfile,
            data: {
                outname: outfile,
                format: "json",
            }
        }, nin=1, nout=0),
        ret: cs
    }.ret,

};
// end img.

// local celltreesource = pg.pnode({
//     type: "CelltreeSource",
//     name: "celltreesource",
//     data: {
//         filename: "celltreeOVERLAY.root",
//         EventNo: 6501,
//         // in_branch_base_names: raw [default], calibGaussian, calibWiener
//         in_branch_base_names: ["calibWiener", "calibGaussian"],
//         out_trace_tags: ["wiener", "gauss"], // orig, gauss, wiener
//         in_branch_thresholds: ["channelThreshold", ""],
//     },
//  }, nin=0, nout=1);
local filesource(infile, tags=[]) = pg.pnode({
    type: "FrameFileSource",
    name: infile,
    data: {
        inname: infile,
        tags: tags,
    },
}, nin=0, nout=1);

local dumpframes = pg.pnode({
    type: "DumpFrames",
    name: 'dumpframe',
}, nin=1, nout=0);

local magdecon = pg.pnode({
    type: 'MagnifySink',
    name: 'magdecon',
    data: {
        output_filename: "mag.root",
        root_file_mode: 'UPDATE',
        frames: ['gauss', 'wiener', 'gauss_error'],
        cmmtree: [['bad','bad']],
        summaries: ['gauss', 'wiener', 'gauss_error'],
        trace_has_tag: true,
        anode: wc.tn(anodes[0]),
    },
}, nin=1, nout=1, uses=[anodes[0]]);

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

local frame_quality_tagging = pg.pnode({
    type: 'FrameQualityTagging',
    name: '',
    data: {
        trace_tag: 'gauss',
        anode: wc.tn(anodes[0]),
	nrebin: 4, // rebin count ...
	length_cut: 3,
	time_cut: 3,
	ch_threshold: 100,
	n_cover_cut1: 12,
	n_fire_cut1: 14,
	n_cover_cut2: 6,
	n_fire_cut2: 6,
	fire_threshold: 0.22,
	n_cover_cut3: [1200, 1200, 1800 ],
	percent_threshold: [0.25, 0.25, 0.2 ],
	threshold1: [300, 300, 360 ],
	threshold2: [150, 150, 180 ],
	min_time: 3180,
	max_time: 7870,
	flag_corr: 1,
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
// multi slicing includes 2-view tiling and dead tiling
local active_planes = [[0,1,2],[0,1],[1,2],[0,2],];
local masked_planes = [[],[2],[0],[1]];
// single, multi, active, masked
function(infile="celltreeOVERLAY-event6501.tar.bz2",
         outfile="clusters-event6501.tar.gz",
         slicing = "single")

    local multi_slicing = slicing;
local imgpipe =
    if multi_slicing == "single" then
        pg.pipeline([
            img.slicing(anode, anode.name, "gauss", 109, active_planes=[0,1,2], masked_planes=[],dummy_planes=[]), // 109*22*4
            img.tiling(anode, anode.name),
            img.solving(anode, anode.name),
            // img.clustering(anode, anode.name),
        ], "img-" + anode.name)
    else if multi_slicing == "active" then
        pg.pipeline([
            img.multi_active_slicing_tiling(anode, anode.name+"-ms-active", "gauss", 4),
            img.solving(anode, anode.name+"-ms-active")
        ])
    else if multi_slicing == "masked" then
        pg.pipeline([
            // img.multi_masked_slicing_tiling(anode, anode.name+"-ms-masked", "gauss", 109),
            img.multi_masked_2view_slicing_tiling(anode, anode.name+"-ms-masked", "gauss", 109),
            img.clustering(anode, anode.name+"-ms-masked"),
        ])
    else {
        local active_fork = pg.pipeline([
            img.multi_active_slicing_tiling(anode, anode.name+"-ms-active", "gauss", 4),
            img.solving(anode, anode.name+"-ms-active"),
        ]),
        local masked_fork = pg.pipeline([
            // img.multi_masked_slicing_tiling(anode, anode.name+"-ms-masked", "gauss", 109),
            img.multi_masked_2view_slicing_tiling(anode, anode.name+"-ms-masked", "gauss", 109),
            img.clustering(anode, anode.name+"-ms-masked"),
        ]),
        ret: pg.fan.fanout("FrameFanout",[active_fork,masked_fork], "fan_active_masked"),
    }.ret;
local graph = pg.pipeline([
    filesource(infile, tags=["gauss","wiener"]),
    // frame_quality_tagging, // event level tagging
    // cmm_mod, // CMM modification
    // frame_masking, // apply CMM
    charge_err, // calculate charge error
    // magdecon, // magnify out
    // dumpframes,
    imgpipe,
    img.dump(outfile),
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
        plugins: ["WireCellGen", "WireCellPgraph", "WireCellSio", "WireCellSigProc", "WireCellRoot", "WireCellImg"],
        apps: ["Pgrapher"]
    }
};

[cmdline] + pg.uses(graph) + [app]
