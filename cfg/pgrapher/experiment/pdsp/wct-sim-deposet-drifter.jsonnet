local g = import 'pgraph.jsonnet';
local f = import 'pgrapher/common/funcs.jsonnet';
local wc = import 'wirecell.jsonnet';

local io = import 'pgrapher/common/fileio.jsonnet';
local tools_maker = import 'pgrapher/common/tools.jsonnet';
local params = import 'pgrapher/experiment/pdsp/simparams.jsonnet';

local tools = tools_maker(params);

local sim_maker = import 'pgrapher/experiment/pdsp/sim.jsonnet';
local sim = sim_maker(params, tools);

local stubby = {
  tail: wc.point(3000.0, 1000.0, 0.0, wc.mm),
  head: wc.point(3000.0, 1000.0, 2300.0, wc.mm),
};

local tracklist = [

  {
    time: 0 * wc.us, 
    charge: -5000, // 5000 e/mm
    ray: stubby, // params.det.bounds,
  },

];
local output = 'wct-sim-ideal-sig.npz';


//local depos = g.join_sources(g.pnode({type:"DepoMerger", name:"BlipTrackJoiner"}, nin=2, nout=1),
//                             [sim.ar39(), sim.tracks(tracklist)]);
local depos = sim.tracks(tracklist, step=1.0 * wc.mm);
local deposet = g.pnode({
        type: "TrackDepoSet",
        name: "trackdeposet",
        data: {
            step_size: 1.0*wc.mm,
            tracks: tracklist,
        }
    }, nin=0, nout=1);


//local deposio = io.numpy.depos(output);
local drifter = sim.drifter;
local setdrifter = g.pnode({
            type: 'DepoSetDrifter',
            data: {
                drifter: "Drifter"
            }
        }, nin=1, nout=1, uses=[drifter]);
local bagger = sim.make_bagger();
// signal plus noise pipelines
//local sn_pipes = sim.signal_pipelines;
local sn_pipes = sim.splusn_pipelines;

local multimagnify = import 'pgrapher/experiment/pdsp/multimagnify.jsonnet';
local magoutput = 'case3.root';

local multi_magnify = multimagnify('orig', tools, magoutput);
local magnify_pipes = multi_magnify.magnify_pipelines;
local multi_magnify2 = multimagnify('raw', tools, magoutput);
local magnify_pipes2 = multi_magnify2.magnify_pipelines;
local multi_magnify3 = multimagnify('gauss', tools, magoutput);
local magnify_pipes3 = multi_magnify3.magnify_pipelines;
local multi_magnify4 = multimagnify('wiener', tools, magoutput);
local magnify_pipes4 = multi_magnify4.magnify_pipelines;

local perfect = import 'pgrapher/experiment/pdsp/chndb-base.jsonnet';
local chndb = [{
  type: 'OmniChannelNoiseDB',
  name: 'ocndbperfect%d' % n,
  data: perfect(params, tools.anodes[n], tools.field, n),
  uses: [tools.anodes[n], tools.field],  // pnode extension
} for n in std.range(0, std.length(tools.anodes) - 1)];

//local chndb_maker = import 'pgrapher/experiment/pdsp/chndb.jsonnet';
//local noise_epoch = "perfect";
//local noise_epoch = "after";
//local chndb_pipes = [chndb_maker(params, tools.anodes[n], tools.fields[n]).wct(noise_epoch)
//                for n in std.range(0, std.length(tools.anodes)-1)];
local nf_maker = import 'pgrapher/experiment/pdsp/nf.jsonnet';
// local nf_pipes = [nf_maker(params, tools.anodes[n], chndb_pipes[n]) for n in std.range(0, std.length(tools.anodes)-1)];
local nf_pipes = [nf_maker(params, tools.anodes[n], chndb[n], n, name='nf%d' % n) for n in std.range(0, std.length(tools.anodes) - 1)];

local sp_maker = import 'pgrapher/experiment/pdsp/sp.jsonnet';
local sp = sp_maker(params, tools);
local sp_pipes = [sp.make_sigproc(a) for a in tools.anodes];

//local parallel_pipes = [g.pipeline([sn_pipes[n], magnify_pipes[n],
//                                nf_pipes[n], magnify_pipes2[n] ], "parallel_pipe_%d"%n)
//                    for n in std.range(0, std.length(tools.anodes)-1)];
local parallel_pipes = [
  g.pipeline([
               sn_pipes[n],
               // magnify_pipes[n],
               // nf_pipes[n],
               // magnify_pipes2[n],
               sp_pipes[n],
               magnify_pipes3[n],
               // magnify_pipes4[n],
             ],
             'parallel_pipe_%d' % n)
  for n in std.range(0, std.length(tools.anodes) - 1)
];
local outtags = ['raw%d' % n for n in std.range(0, std.length(tools.anodes) - 1)];
local parallel_graph = f.fanpipe('DepoSetFanout', parallel_pipes, 'FrameFanin', 'sn_mag_nf', outtags);


//local frameio = io.numpy.frames(output);
local sink = sim.frame_sink;

// local graph = g.pipeline([depos, drifter, bagger, parallel_graph, sink]);
local plainbagger = g.pnode({
        type:'DepoBagger',
        name:'plainbagger',
        data: {
            gate: [0, 0],
        },
    }, nin=1, nout=1);

local deposet_rotate = g.pnode({
        type:'DepoSetRotate',
        name:'deposet_rotate',
        data: {
            rotate: true,
            transpose: [1,0,2],
            scale: [-1,1,1],
        },
    }, nin=1, nout=1);

// FIXME: need a "bagger" after "setdrifter" for trimming out-of-window depos
// local graph = g.pipeline([depos, plainbagger, setdrifter, parallel_graph, sink]);
local graph = g.pipeline([depos, plainbagger, deposet_rotate, setdrifter, parallel_graph, sink]);

local app = {
  type: 'Pgrapher',
  data: {
    edges: g.edges(graph),
  },
};

local cmdline = {
    type: "wire-cell",
    data: {
        plugins: ["WireCellGen", "WireCellPgraph", "WireCellSio", "WireCellSigProc", "WireCellRoot"],
        apps: ["Pgrapher"]
    }
};


// Finally, the configuration sequence which is emitted.

[cmdline] + g.uses(graph) + [app]
