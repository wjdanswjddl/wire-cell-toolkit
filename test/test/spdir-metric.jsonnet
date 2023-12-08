// This is a top level config file given to wire-cell to run segments of
// processing to calcualte the "standard" SP metrics (eff, bias, res) vs
// direction of ideal tracks.
//
// The caller defines a pipeline of tasks that default to sim,nf,sp.  A subset
// of this task list may be given.  Eg, UVCGAN may be inserted to perform domain
// translations and that may reasonably be done before or after the nf stage.
//
// Example usage:
// 
// wire-cell -c spdir-metric.jsonnet \
//           -A detector=pdsp \
//           -A variant=nominal \
//           -A input=depos.npz \
//           -A output=sp.npz \
//           -A tasks=sim,nf,sp

local high = import "layers/high.jsonnet";
local wc = high.wc;
local pg = high.pg;

// Main pipeline elements.
local builder(mid, anode, stages, outputs) = {
    local last_stage = stages[std.length(stages)-1],
    main : {
        drift: [
            mid.drifter(),
        ],
        splat: [
            // nothing, splat itself is part of the sink branch
        ],
        sim: [
            mid.sim.signal(anode),
            mid.sim.noise(anode),
            mid.sim.digitizer(anode),
        ],
        nf: [
            mid.sigproc.nf(anode),
        ],
        sp: [
            mid.sigproc.sp(anode),
        ]
    },
    sinks: {
        drift: high.fio.depo_file_sink(outputs.drift),
        splat: pg.pipeline([
            mid.sim.splat(anode),        
            high.fio.frame_file_sink(outputs.splat),
        ], name='splat'+std.toString(anode.data.ident)),
        sim: high.fio.frame_file_sink(outputs.sim),
        nf: high.fio.frame_file_sink(outputs.nf),
        sp: high.fio.frame_file_sink(outputs.sp),
    },

    tap_or_sink(stage):
        if stage == last_stage then
            self.sinks[stage]
        else
            high.fio.tap(if stage == "drift" || stage == "splat" then "DepoSetFanout" else "FrameFanout",
                         self.sinks[stage]),
        
    get_stage(stage):
        if std.objectHas(outputs, stage) then
            self.main[stage] + [self.tap_or_sink(stage)]
        else
            self.main[stage],
};
    

local source(stage, input) = 
    if stage == "drift"
    then high.fio.depo_file_source(input)
    else high.fio.frame_file_source(input);


local output_objectify(stages, output) =
    local last_stage = stages[std.length(stages)-1];
    if std.type(output) == "string" then
        {[last_stage]: output}
    else output;

// Return configuration for single-APA job.
// - detector :: canonical name of supported detector (pdsp, uboone, etc)
// - input :: name of file provding data to input to first task
// - ouput :: name of file to receive output of last task or object mapping task to output file 
// - task :: array or comma separated list of task names
function (detector, input, output, tasks="drift,splat,sim,nf,sp", variant="nominal")
    local mid = high.mid(detector, variant);
    local stages = wc.listify(tasks);
    local outfiles = output_objectify(stages, output); // stage->filename

    local anodes = mid.anodes();
    local anode = anodes[0];

    local b = builder(mid, anode, stages, outfiles);

    local head = [source(stages[0], input)];
    local guts = [b.get_stage(stage) for stage in stages];
    local body = std.flattenArrays(guts);

    local graph = pg.pipeline(head + body);
    high.main(graph, "Pgrapher")

