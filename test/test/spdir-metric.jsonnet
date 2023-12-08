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

local task_builder(mid, anode) = {
    drift: [
        mid.drifter()
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
        mid.sigproc.sp(anode)
    ]
};

local source(stage, input) = 
    if stage == "drift"
    then high.fio.depo_file_source(input)
    else high.fio.frame_file_source(input);

local sink(stage, output) = 
    if stage == "drift"
    then high.fio.depo_file_sink(output)
    else high.fio.frame_file_sink(output);

local tap(stage, sink) =
    if stage == "drift"
    then high.fio.tap("DepoSetFanout", sink)
    else high.fio.tap("FrameFanout", sink);

// output is string giving a file name or an object mapping a stage to a file name.
// return an object mapping a stage to an output pipeline (array of nodes)
local parse_taps(stages, output) =
    local last_stage = stages[std.length(stages)-1];
    if std.type(output) == "string" then
        {last_stage: [sink(last_stage, output)]}
    else
        {[stage]: 
            if stage == last_stage
            then [sink(stage, output[stage])]
            else [tap(stage, sink(stage, output[stage]))]
         for stage in std.objectFields(output)}
    ;


// Return configuration for single-APA job.
// - detector :: canonical name of supported detector (pdsp, uboone, etc)
// - input :: name of file provding data to input to first task
// - ouput :: name of file to receive output of last task or object mapping task to output file 
// - task :: array or comma separated list of task names
function (detector, input, output, tasks="drift,sim,nf,sp", variant="nominal")
    local stages = wc.listify(tasks);

    local taps = parse_taps(stages, output); // stage->[list,of,nodes]

    local mid = high.mid(detector, variant);

    local anodes = mid.anodes();
    local anode = anodes[0];
    local tb = task_builder(mid, anode); // stage->[list,of,nodes]

    local head = [source(stages[0], input)];
    local guts = [tb[stage] + std.get(taps, stage, []) for stage in stages];
    local body = std.flattenArrays(guts);
    local graph = pg.pipeline(head + body);
    high.main(graph, "Pgrapher")

