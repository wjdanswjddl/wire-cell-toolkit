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


function (detector, input, output, tasks="sim,nf,sp", variant="nominal")
    local stages=std.split(tasks,",");
    local mid = high.mid(detector, variant);

    local source = [if stages[0] == "sim"
                    then high.fio.depo_file_source(input)
                    else high.fio.frame_file_source(input)];
    local sink = [high.fio.frame_file_sink(output)];

    local anodes = mid.anodes();
    local anode = anodes[0];
    local tb = task_builder(mid, anode);
    local body = std.flattenArrays([tb[stage] for stage in stages]);
    local graph = pg.pipeline(source + body + sink);
    high.main(graph, "Pgrapher")

    
