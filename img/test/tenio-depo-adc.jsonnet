// Configure wire-cell CLI to run a job that reads depos from file
// applies simulation and writes results to file.

local high = import "layers/high.jsonnet";
local wc = high.wc;
local pg = high.pg;

function(detector, variant="nominal",
         indepos=null,
         outdepos="depos-drifted.npz",
         outfiles="frames-adc-%(anode)s.npz",
         frame_mode="sparse", anode_iota=null)

    local mid = high.mid(detector, variant, options={sparse:false});

    local source = if std.type(indepos) == "null"
                  then mid.sim.track_depos()
                  else high.fio.depo_file_source(indepos);

    local drifter = mid.drifter();

    local anodes = mid.anodes();
    local iota = if std.type(anode_iota) == "null" then std.range(0, std.length(anodes)-1) else anode_iota;

    local apipes = [
        local anode = anodes[aid];
        local ofile = std.format(outfiles, {anode: anode.data.ident});
        pg.pipeline([
            mid.sim.signal(anode),
            mid.sim.noise(anode),
            mid.sim.digitizer(anode),

            high.fio.frame_tensor_file_sink(ofile,mode=frame_mode, digitize=true)

        ]) for aid in iota];

    local body = pg.fan.fanout('DepoSetFanout', apipes, "work");

    local graph = pg.pipeline([source, drifter, body], "main");
    local executor = "TbbFlow";
    // local executor = "Pgrapher";
    high.main(graph, executor)



