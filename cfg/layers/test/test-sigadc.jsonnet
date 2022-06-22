// Generate ideal track depos, simulate signal ADC (no noise).

local high = import "layers/high.jsonnet";

function(detector, variant="nominal", 
         platform = 'cpu',
         output="sim-%(type)s-anode%(anode)d.npz") 

    local svc = high.services(platform);
    local mid = high.mid(detector, variant, svc);

    local depos = mid.sim.track_depos();
    local drifter = mid.drifter();
    local anodes = mid.anodes();
    local apipes = [
        high.pg.pipeline([
            mid.sim.signal(anode),
            mid.sim.digitizer(anode),
            high.fio.frame_file_sink(std.format(output, {
                type:"sigadc", anode: anode.data.ident})),
        ]),
        for anode in anodes];

    local body = high.pg.fan.fanout('DepoSetFanout', apipes);
    local graph = high.pg.pipeline([depos, drifter, body], "main");
    high.main(graph, 'TbbFlow')
    

