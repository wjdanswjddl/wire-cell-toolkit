local pg = import "pgraph.jsonnet";

local pipes = [
    pg.pnode({
        type:"Test",
        name:"test%d"%n,
        data:{number:n,index:n-1}
    }, nin=1, nout=1) for n in std.range(1,6)];
local fpg = pg.fan.pipe('TestFanout', pipes, 'TestFanin', name="testpipe", outtags=["testtag"]);
local fpga = { type: "Pgrapher", data: { edges: pg.edges(fpg), }, };

// make something looking like a config so dotify works
pg.uses(fpg) + [ fpga ]


