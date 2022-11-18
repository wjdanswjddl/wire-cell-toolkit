#!/usr/bin/env python
'''
Functions for use in adc-noise-sig.smake
'''
import numpy
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from numpy.ma import masked_array, masked_where, masked_invalid

# jsonnet -J cfg -e 'local a=import "test/pdsp-adc.jsonnet"; a.voltageperadc'
voltageperadc=3.4179687499999998e-10
# Why is toffset not exactly tbin?
toffset=873

def load_array(fname, index=0, with_name=False):
    fp = numpy.load(fname)
    name = list(fp.keys())[index]
    arr = fp[name]
    if with_name:
        return arr, name
    return arr

def transform_array(orig, copy, trans=numpy.round, dtype='f2'):
    arr, name = load_array(orig, with_name=True)
    arr = numpy.array(trans(arr), dtype=dtype)
    numpy.savez_compressed(copy, **{name:arr})

adcmm=10
voltmm=adcmm*voltageperadc

def plot_params(source, tier, domain):
    'Return dictionary of plotting parameters based wildcards'

    ## gleen pedestal via:
    # wcsonnet cfg/test/pdsp-adc.jsonnet | jq '.adcpeds[0]'

    pp = dict(
        source=source, tier=tier, domain=domain,
        pedestal=0,
        #cmap='seismic',
        cmap='tab20',
        tickslice=slice(0, 256),
        aname = dict(raw='arr_0', rnd='arr_0', vlt='frame_raw_0',
                     src='frame_*_0', ref='frame_*_0', noi='frame_*_0', dig='frame_*_0',
                     sig='frame_gauss0_0')[tier],
        tier_title = dict(raw='Raw',
                          rnd='Rounded',
                          vlt='Scaled',
                          src='Loaded',
                          ref='Reframed',
                          noi='Noise addded',
                          dig='Digitized, no pedestal',
                          sig='Signal')[tier],
        tier_units = dict(raw='ADC',
                          rnd='ADC',
                          vlt='voltage',
                          src='voltage',
                          ref='voltage',
                          noi='voltage',
                          dig='ADC',
                          sig='electrons')[tier],
        source_title = dict(real="Sim",
                            fake="GAN",
                            diff="%-diff",
                            xddp="%-diff"
                            )[source],
        domain_title = dict(a="q1d", b="2d",
                            ab="q1d / 2d", ba="2d / q1d")[domain],
    )

    if tier in ('raw', 'rnd', 'dig'):
        pp['vmin'] = -adcmm
        pp['vmax'] = adcmm
    if tier in ('sig', ):
        pp['vmin'] = 0
        pp['vmax'] = 2000
        pp['masked_value'] = 0
        pp['cmap'] = 'jet'
    if tier in ('vlt','ref', 'noi', 'src'):
        pp['vmin'] = -voltmm
        pp['vmax'] = voltmm

    if tier in ('ref','noi', 'dig', 'sig'):
        pp['tickslice'] = slice(toffset, toffset+256)
    if tier in ('dig'):
        pp['pedestal']=2350

    ts = pp['tickslice']
    pp['tickrange'] = f'{ts.start}:{ts.stop}'
    pp['zoomslice'] = (slice(0,256), ts)
    pp['zoomrange'] = f'0:256,{ts.start}:{ts.stop}'

    if source == "diff":
        pp['pedestal']=0
        pp['vmin'] = -10.0      # percent
        pp['vmax'] = 10.0
        pp['cmap']='jet' #'tab20'
    if source == "xddp":
        pp['vmin'] = -50.0      # percent
        pp['vmax'] = 50.0
        pp['cmap']='jet'
    return pp
    

def render_params(w):
    pp = plot_params(w.source, w.tier, w.domain)
    args = "-c {cmap} -a '{aname}' --vmin {vmin} --vmax {vmax} -z '{zoomrange}' -b {pedestal}"
    return args.format(**pp)

masked_value = numpy.NaN

def make_diffarr(wc, arrs, pars):
    aarr, barr = arrs
    apar, bpar = pars
    rmf = (barr - aarr)
    den = 0.5*(numpy.abs(barr-bpar["pedestal"])+numpy.abs(aarr-apar["pedestal"]))
    nz = den != 0
    da = numpy.zeros_like(rmf)
    da[nz] = (100.0*rmf[nz]) / den[nz]

    if wc.tier in ("dig",):
        da[den==0] = 0          # turn divide by zero into zero 
    else:                       # o.w. supress very small denominator
        nz = den > 0.0001*numpy.max(den)
        da[~nz] = masked_value
    return da

# def diffarr(wc, fake, real, diff):
#     pars = [plot_params("fake", wc.tier, wc.domain),
#             plot_params("real", wc.tier, wc.domain)]
#     ra,rname = load_array(real, with_name=True)
#     fa,fname = load_array(fake, with_name=True)
#     arrs = [fa, ra]
#     da = make_diffarr(wc, arrs, parrs)
#     numpy.savez_compressed(diff, **{rname:da})

def plot_diff(wc, files, params):
    apar,bpar,dpar = params
    aarr = load_array(files[0])
    barr = load_array(files[1])
    darr = make_diffarr(wc, [aarr,barr], [apar,bpar])
    
    fig, axes = plt.subplots(nrows=1, ncols=3, sharex=True, sharey=True)

    common = dict(aspect='auto')
    stit = '{tier_title} ({domain_title}) [{tier_units}] (sample {event})'.format(event=wc.event, **dpar)
    fig.suptitle(stit)
    ims = list()
    for ind, (pp,aa) in enumerate(zip(params,[aarr,barr,darr])):
        aa = aa[pp['zoomslice']] - pp['pedestal']
        if 'masked_value' in pp:
            aa[aa==pp['masked_value']] = numpy.NaN

        ax = axes[ind]
        ax.get_xaxis().set_visible(False);
        ax.get_yaxis().set_visible(False);
        tit = pp['source_title']
        ax.set_title(tit)
        im = ax.imshow(masked_invalid(aa),
                       vmin=pp['vmin'], vmax=pp['vmax'],
                       cmap=pp['cmap'], aspect='auto')
        ims.append(im)
    # fig.subplots_adjust(left=0.2, right=0.8)
    # cax_val = fig.add_axes([0.05, 0.15, 0.05, 0.7])
    # cax_dif = fig.add_axes([0.85, 0.15, 0.05, 0.7])
    # fig.colorbar(ims[0], cax=cax_val)
    # fig.colorbar(ims[2], cax=cax_dif)
    fig.colorbar(ims[0], ax=axes[0:2], location='left')
    fig.colorbar(ims[2], ax=axes[2:])

    # plt.savefig(files[2], bbox_inches='tight')
    plt.savefig(files[2])


def plot_frdp(wc, fake, real, plot):
    fp = plot_params("fake", wc.tier, wc.domain)
    rp = plot_params("real", wc.tier, wc.domain)
    dp = plot_params("diff", wc.tier, wc.domain)
    plot_diff(wc, [fake,real,plot], [fp, rp, dp])

def plot_xddp(wc, afile, bfile, pfile):
    ap = plot_params(wc.source, wc.tier, "a")
    bp = plot_params(wc.source, wc.tier, "b")
    dp = plot_params("xddp", wc.tier, "ab")
    plot_diff(wc, [afile, bfile, pfile], [ap, bp, dp])


