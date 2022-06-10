// A contrived noise model suitable for GroupNoiseModel

local wc = import "wirecell.jsonnet";

local ngrps = 10;

// Generate a totally bogus spectrum for a given group.  The higher
// the group number the more the noise energy.
local genspec = function(grp, npts=32, nyquist=wc.megahertz) {
    local arbitrary = 0.0005,   // tune to get sort of 10-20 ADC for noisiest 
    local weight = arbitrary*(grp+1)/ngrps,
    nyquist: nyquist,
    groupID: grp,
    freqs: [ind*(nyquist/npts) for ind in std.range(0, npts)],
    amps: [weight*std.pow(freq/nyquist, 2) * std.exp(-20*(freq/nyquist)) for freq in self.freqs],
};

[ genspec(grp) for grp in std.range(0, ngrps-1) ]
