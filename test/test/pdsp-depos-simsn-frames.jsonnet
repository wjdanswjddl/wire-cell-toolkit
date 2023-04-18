// This is used by test-pdsp-simsn.bats.

local pg = import 'pgraph.jsonnet';
local wc = import 'wirecell.jsonnet';
local io = import 'pgrapher/common/fileio.jsonnet';

local tools_maker = import 'pgrapher/common/tools.jsonnet';
local base = import 'pgrapher/experiment/pdsp/simparams.jsonnet';
local params = base {
  lar: super.lar {
    // Longitudinal diffusion constant
    // DL: std.extVar('DL') * wc.cm2 / wc.s,
    DL: 4.0 * wc.cm2 / wc.s,
    // Transverse diffusion constant
    // DT: std.extVar('DT') * wc.cm2 / wc.s,
    DT: 8.8 * wc.cm2 / wc.s,
    // Electron lifetime
    // lifetime: std.extVar('lifetime') * wc.ms,
    lifetime: 10.4 * wc.ms,
    // Electron drift speed, assumes a certain applied E-field
    // drift_speed: std.extVar('driftSpeed') * wc.mm / wc.us,
    drift_speed: 1.565 * wc.mm / wc.us,
  },
};

local tools = tools_maker(params);

// We only use first anode.
local anode = tools.anodes[0];
local pirs = tools.pirs[0];

/// Try to make as much locally defined and not define on external Jsonnet.
// local sim_maker = import 'pgrapher/experiment/pdsp/sim.jsonnet';
// local sim = sim_maker(params, tools);
local sim_maker = import "pgrapher/common/sim/nodes.jsonnet";
local sim = sim_maker(params, tools);

local drifter = sim.drifter;
local setdrifter = pg.pnode({
    type: 'DepoSetDrifter',
    data: {
        drifter: "Drifter"
    }
}, nin=1, nout=1, uses=[drifter]);

local transform =  pg.pnode({
    type:'DepoTransform',
    name: "",
    data: {
        rng: wc.tn(tools.random),
        dft: wc.tn(tools.dft),
        anode: wc.tn(anode),
        pirs: std.map(function(pir) wc.tn(pir), pirs),
        fluctuate: false,   // minimize randomness
        drift_speed: params.lar.drift_speed,
        first_frame_number: 100, // just to be different....
        readout_time: params.sim.ductor.readout_time,
        start_time: params.sim.ductor.start_time,
        tick: params.daq.tick,
        nsigma: 3,
    },
}, nin=1, nout=1, uses=[anode, tools.random, tools.dft] + pirs);
local reframer = pg.pnode({
    type: 'Reframer',
    name: '',
    data: {
        anode: wc.tn(anode),
        tbin: params.sim.reframer.tbin, // 120
        nticks: params.sim.reframer.nticks,
    },
}, nin=1, nout=1);
local noise_model = {
    type: "EmpiricalNoiseModel",
    name: "",
    data: {
        anode: wc.tn(anode),
        dft: wc.tn(tools.dft),
        chanstat: "",
        spectra_file: params.files.noise,
        nsamples: params.daq.nticks,
        period: params.daq.tick,
        wire_length_scale: 1.0*wc.cm, // optimization binning
    },
    uses: [anode, tools.dft],
};
local addnoise = pg.pnode({
    type: "AddNoise",
    name: "",
    data: {
        rng: wc.tn(tools.random),
        dft: wc.tn(tools.dft),
        model: wc.tn(noise_model),
	nsamples: params.daq.nticks,
        replacement_percentage: 0.02, // random optimization
    }}, nin=1, nout=1, uses=[tools.random, tools.dft, noise_model]);

local digitizer = pg.pnode({
    type: "Digitizer",
    name: "",
    data : params.adc {
        anode: wc.tn(anode),
        frame_tag: "orig",
    }
}, nin=1, nout=1, uses=[anode]);



local depo_source(input)  = pg.pnode({
    type: 'DepoFileSource',
    name: wc.basename(input),
    data: { inname: input } // "depos.tar.bz2"
}, nin=0, nout=1);

local sio_sink(output, tags=[]) = pg.pnode({
    type: "FrameFileSink",
    name: wc.basename(output),
    data: {
        outname: output, // "frames.tar.bz2",
        tags: tags,
        digitize: false,
    },
}, nin=1, nout=0);

local sio_tap(output) = pg.fan.tap('FrameFanout', sio_sink(output), wc.basename(output));

// signal plus noise pipelines
// local sn_pipes = sim.signal_pipelines;
// local sn_pipes = sim.splusn_pipelines;
local sig = pg.pipeline([setdrifter, transform, reframer]);
local noi = pg.pipeline([addnoise, digitizer]);


local make_graph(input, sigoutput, adcoutput) =
    pg.pipeline([depo_source(input), sig, sio_tap(sigoutput), noi, sio_sink(adcoutput, ["orig"])]);
local plugins = [ "WireCellSio", "WireCellGen", "WireCellSigProc","WireCellApps", "WireCellPgraph", "WireCellTbb", "WireCellRoot"];

function(input, sigoutput, adcoutput) 
    local graph = make_graph(input, sigoutput, adcoutput);
    // Pgrapher or TbbFlow
    local engine = "Pgrapher";
    local app = {
        type: 'Pgrapher',
        data: {
            edges: pg.edges(graph),
        },
    };
    local cmdline = {
        type: "wire-cell",
        data: {
            plugins: plugins,
            apps: ["Pgrapher"],
        }
    };
    // Finally, the configuration sequence which is emitted.
    [cmdline] + pg.uses(graph) + [app]
