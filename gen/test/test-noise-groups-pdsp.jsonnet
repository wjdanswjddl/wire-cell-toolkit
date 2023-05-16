// coherent group noise for PDSP
local one(gid, start, num) = {
    plane: "p" + std.toString(gid),
    groupID: gid,
    channels: std.range(start, start+num-1),
};
[
    one(1, 0, 400),
    one(2, 400, 400),
    one(3, 800, 400),
    one(4, 1200, 400),
    one(5, 1600, 480),
    one(6, 2080, 480),    
]
