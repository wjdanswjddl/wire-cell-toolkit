// A contrived noise model suitable for GroupNoiseModel

local wc = import "wirecell.jsonnet";

// The Rayleigh PDF is shaped similar to real world noise
local rayleigh = function(x, sigma=0.1)
    x / (sigma*sigma) * std.exp(-0.5*std.pow(x/sigma, 2));

// Generate a bogus but roughly realistic spectrum for a given group
// number grp out of ngrps and for .
// 
// "nsamples" is how many "regular" samples ("ticks") that the
// "original" waveforms had which were used to form the mean
// amplitude.  The freqs/amps arrays returned will have arbitrary size
// smaller than "nsamples" (by the sqrt).  May tune "knob" to get some
// rough noise level and "scale" effectively sets to units of measure.
local genspec = function(
    // The group number (arb)
    grpnum,
    // The number of samples (ticks) in the (fictional) waveforms
    // leading to the returned spectrum.
    nsamples, 
    // The period of those samples
    period,
    // The location of the spectral peak expressed as a fraction of
    // the Nyquist frequency = 0.5/period.
    fpeak,
    // A scale multiplied to the raw spectrum in order to provide
    // correct units of measure,
    scale,
    // Number of points out of nsamples to save
    nsave)
    {
    
        // The Rayleigh frequency / frequency bin size
        local fstep = 0.5/period,
        nsamples: nsamples,
        period: period,
        groupID: grpnum,
        freqs: [ind*fstep for ind in std.range(0, nsave-1)],
        amps: [scale*rayleigh(freq, 0.5*fpeak/period) for freq in $.freqs],
    };


// See above for parameter docstrings
function(ngrps=10, nsamples=4096, nsave=64, period=0.5*wc.us, fpeak=0.1, scale=10*wc.volt) [
    genspec(grp, nsamples, period, fpeak, scale*grp/ngrps, nsave) for grp in std.range(1, ngrps)
]
