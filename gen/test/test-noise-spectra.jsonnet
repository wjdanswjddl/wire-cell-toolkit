// A contrived noise model suitable for GroupNoiseModel

local wc = import "wirecell.jsonnet";

local ngrps = 10;

// Generate a spectrum for a given group
local genspec = function(grp, npts=10, nyquist=wc.megahertz) {
    local weight = (grp+1)/ngrps,
    nyquist: nyquist,
    groupID: grp,
    freqs: [ind*(nyquist/npts) for ind in std.range(0, npts)],
    amps: [weight*4000*std.pow(freq/nyquist, 2) * std.exp(-20*(freq/nyquist)) for freq in self.freqs],
};

[ genspec(grp) for grp in std.range(0, ngrps-1) ]
