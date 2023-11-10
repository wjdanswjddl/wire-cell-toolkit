#!/usr/bin/env python
'''
Functions for use in adc-noise-sig-bidir.smake

This is a dup+hack on adc_noise_sig.py with various taxonomy shifts.

'''
import os
import numpy
import zipfile
from pathlib import Path
from collections import defaultdict, namedtuple
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
from numpy.ma import masked_array, masked_where, masked_invalid

# jsonnet -J cfg -e 'local a=import "test/pdsp-adc.jsonnet"; a.voltageperadc'
voltageperadc=3.4179687499999998e-10
# Why is toffset not exactly tbin?
toffset=873

def load_array(fname, index=0, with_name=False):
    if type(fname) != str:
        return fname

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


def coalesce(fnames, out):
    '''Bring together arrays of common origin/generator/domain/tier to one file.

    In the process re-crop to match original patch.
    '''
    ct = coal_tuple(out)

    size = 256
    t0 = dict(dig=toffset, sig=toffset, vlt=0).get(ct.tier, 0)

    garbage = list()

    with zipfile.ZipFile(out, mode='w', compression=zipfile.ZIP_DEFLATED) as zf:
        for fname in fnames:
            base = os.path.splitext(fname)[0]
            aname = "_".join(base.split("/")[-5:])
            aname += ".npy"
            # not much faster
            # afile = f'/dev/shm/{aname}'
            afile = f'/tmp/{aname}'
            #afile = aname
            garbage.append(afile)

            fp = numpy.load(fname);
            keys = list(fp.keys())

            frame_names = [k for k in keys if k.startswith("frame_")]
            assert len(frame_names) == 1
            frame_name = frame_names[0]
            arr = fp[frame_name]
            crop = arr[0:size, t0:t0+size]
            tot0 = hash(str(arr.tobytes()))
            tot1 = hash(str(crop.tobytes()))
            # print(f'coalescing {fname}:{frame_name}: {arr.shape} {tot0}')
            # print(f'           {out}:{aname}: {t0}@{crop.shape} {tot1}')

            numpy.save(afile, crop)
            zf.write(afile, aname)
    for die in garbage:
        os.remove(die)
        

def coal_aname(wc, **kwds):
    '''
    Form coalesced array name based on object and any overriding keywords.
    '''
    d = wc._asdict()
    d.update(kwds)
    return "{origin}_{generator}_{domain}_{sample}_{tier}".format(**d)

def coal_dict(aname):
    parts = aname.split("_")
    return dict(origin=parts[0],
                generator=parts[1],
                domain=parts[2],
                sample=parts[3],
                tier=parts[4]);
Coal = namedtuple("Coal", "origin generator domain sample tier")
def coal_tuple(aname):
    aname,ext = os.path.splitext(aname)
    if "/" in aname:            # path
        parts = aname.split("/")[-5:]
    else:                       # array name
        parts = aname.split("_")
    return Coal(*parts)


# A variety of names for the same arrays got used
generator_array_name = dict(
    WCSim="arr_0",
    UVCGAN="arr_0",
    ACLGAN="data",
    UGATIT="data",
    CycleGAN="x",
)
def plot_params(w, **kwds):
    'Return dictionary of plotting parameters based wildcards'

    def get(name, default=None):
        return getattr(w, name, kwds.get(name, default))

    ## gleen pedestal via:
    # wcsonnet cfg/test/pdsp-adc.jsonnet | jq '.adcpeds[0]'

    source = get("source", 'pimp') # fixme: there are more
    tier = get("tier")
    domain = get("domain")
    generator = get("generator")

    array_name = generator_array_name[generator];

    pp = dict(
        tier=tier,
        domain=domain,
        pedestal=0,
        #cmap='seismic',
        cmap='tab20',
        tickslice=slice(0, 256),
        aname = dict(raw=array_name, rnd=array_name, vlt='frame_raw_0',
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
        source_title = dict(orig=generator,
                            real="Sim",
                            fake="GAN",
                            diff="%-diff",
                            xddp="%-diff",
                            pimp="Paired metric"
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
    if source == "pimp":        # paired image metric plot
        pp['vmin'] = -50.0      # percent
        pp['vmax'] = 50.0
        pp['cmap']='jet'
    return pp
    

def render_params(w):
    pp = plot_params(w)
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
    fig.colorbar(ims[0], ax=axes[0:2], location='left')
    fig.colorbar(ims[2], ax=axes[2:])

    # plt.savefig(files[2], bbox_inches='tight')
    plt.savefig(files[2])

# def plot_frdp(wc, images, plot):
#     arrs = numpy.load(images)
#     fake = arrs[coal_aname(wc, source="fake")]
#     real = arrs[coal_aname(wc, source="real")]

#     fp = plot_params(wc.generator, "fake", wc.tier, wc.domain)
#     rp = plot_params(wc.generator, "real", wc.tier, wc.domain)
#     dp = plot_params(wc.generator, "diff", wc.tier, wc.domain)
#     plot_diff(wc, [fake,real,plot], [fp, rp, dp])

# def plot_xddp(wc, afile, bfile, pfile):
#     arrs = numpy.load(images)
#     a = arrs[coal_aname(wc, domain="a")]
#     b = arrs[coal_aname(wc, domain="b")]

#     ap = plot_params(wc.generator, wc.source, wc.tier, "a")
#     bp = plot_params(wc.generator, wc.source, wc.tier, "b")
#     dp = plot_params(wc.generator, "xddp", wc.tier, "ab")
#     plot_diff(wc, [a, b, pfile], [ap, bp, dp])



def dimp(coala, coalb, ofile):
    '''
    Dual image metrics (plotting).

    For each sample, make some metrics that compare across domains.

    '''
    wca = coal_tuple(coala)
    wcb = coal_tuple(coalb)
    print (coala, wca)
    print (coalb, wcb)

    for wc,dom in zip((wca,wcb),("a","b")):
        assert wc.domain == dom
        assert wc.sample == "all"

    assert wca.tier == wcb.tier
    assert wca.generator == wcb.generator

    ap = plot_params(wca)
    bp = plot_params(wcb)

    evs = list()                # sample numbers
    l1s = list()
    l2s = list()

    coala = numpy.load(coala)
    coalb = numpy.load(coalb)

    for aname in coala.keys():
        obja = coal_tuple(aname)
        bname = coal_aname(obja, domain="b")
        # print (f'A={aname}, B={bname}')
        arra = coala[aname]
        arrb = coalb[bname]
        evs.append(obja.sample)
        diff = arrb-arra
        l1s.append(numpy.sum(numpy.abs(diff)))
        l2s.append(numpy.sum((diff)**2))
        
    numpy.savez_compressed(ofile,
                           l1=numpy.array(l1s),
                           l2=numpy.array(l2s),
                           ev=numpy.array(evs))

def pimp(wc, real_fname, fake_fname, pfile):
    '''Paired image metrics (plotting).

    For each sample, make some metrics that compare across 
    real (simulation) and (translation) for a given domain.
    '''
    evs = list()                # sample numbers
    fss = list()                # fake pixel sum
    rss = list()                # real pixel sum
    l1s = list()                # L1 pixel diff
    l2s = list()                # L2 pixel diff

    # arrs["real"]["<eve>"]

    def do_one(fname):
        fp = numpy.load(fname)
        ret = dict()
        for aname in fp.keys():
            cobj = coal_tuple(aname)
            ret[cobj.sample] = (cobj, aname)
        return ret,fp
    real_md, real_arrs = do_one(real_fname);
    fake_md, fake_arrs = do_one(fake_fname);

    reve = list(real_md.keys())
    reve.sort()
    feve = list(fake_md.keys())
    feve.sort()
    if reve != feve:
        raise ValueError("mismatch in sample numbers")

    for eve in reve:
        fo,fn = fake_md[eve]
        fa = fake_arrs[fn];

        ro,rn = real_md[eve]
        ra = real_arrs[rn]

        evs.append(eve)

        ftot = numpy.sum(fa)
        fss.append(ftot)
        rtot = numpy.sum(ra)
        rss.append(rtot)
        l1 = numpy.sum(numpy.abs(fa-ra))
        l1s.append(l1)
        l2 = numpy.sum((fa-ra)**2)
        l2s.append(l2)

    numpy.savez_compressed(pfile,
                           l1=numpy.array(l1s),
                           l2=numpy.array(l2s),
                           ev=evs,
                           fsum=numpy.array(fss),
                           rsum=numpy.array(rss))
    
    
def plot_pimp(wc, ifile, ofile):
    arrs = numpy.load(ifile)

    fig = plt.figure(figsize=(11,8), tight_layout=True)
    gs = GridSpec(2, 6, figure=fig)

    rsum = arrs['rsum']
    fsum = arrs['fsum']

    if wc.tier in ('vlt',):
        rsum *= -1
        fsum *= -1
    if wc.tier in ('dig',):
        # The "dig" pixels measure ADC on top of a finite baseline.  Remove that
        # baseline here to make the sum a bit more "reasonable".  And to hide
        # cancelation due to bipolar shape, take abs.
        blsum = 2346*256*256
        rsum = numpy.abs(rsum - blsum)
        fsum = numpy.abs(fsum - blsum)

    from statistics import covariance, correlation
    cov = covariance(rsum,fsum)
    cor = correlation(rsum,fsum)
    covM = numpy.cov(rsum, fsum);
    print(f'cov={cov} cor={cor} covM=\n{covM}')

    nax = 0
    axes = [
        fig.add_subplot(gs[nax, 0:2]),
        fig.add_subplot(gs[nax, 2:4]),
        fig.add_subplot(gs[nax, 4:6])
    ]

    #fig, axes2 = plt.subplots(nrows=2, ncols=3)

    stit = f'{wc.tier} ({wc.generator}, domain "{wc.domain}")'
    fig.suptitle(stit)

    alpha = 0.5

    for ax in axes:
        ax.set_yscale('log')

    total_tit = "Total"
    if wc.tier in ('dig',):
        total_tit = "Total (abs)"
    axes[0].set_title(total_tit)
    axes[0].hist(rsum, label='Sim', alpha=alpha)
    axes[0].hist(fsum, label='GAN', alpha=alpha)
    # axes[0].get_xaxis().get_major_formatter().set_useOffset(False)
    axes[0].plot([],[],' ', label=f'1-cor={1-cor:.3e}')
    axes[0].legend(framealpha=alpha)

    axes[1].set_title('L1(GAN,Sim)')
    axes[1].hist(arrs['l1'])

    axes[2].set_title('L2(GAN,Sim)')
    axes[2].hist(arrs['l2'])

    nax += 1
    axes = [
        fig.add_subplot(gs[nax, 0:2]),
        fig.add_subplot(gs[nax, 2:4]),
        fig.add_subplot(gs[nax, 4:6])
    ]

    l1 = arrs['l1']
    l2 = arrs['l2']

    from matplotlib import colors
    norm = colors.LogNorm()
    bins2d = 100
    axes[0].set_title("totals")
    axes[0].set_xlabel("simulated total")
    axes[0].set_ylabel("translated total")
    # axes[0].ticklabel_format(useOffset=False)
    axes[0].hist2d(rsum, fsum, bins=bins2d, norm=norm)

    def plot_l(arrs, lname, ax):
        l = arrs[lname]
        l_okay = l > 0

        x = rsum[l_okay]
        y = numpy.log10(l[l_okay])
        tit = lname.upper()
        ax.set_xlabel("simulated total")
        ax.set_title(tit + " (log10)")
        # ax.ticklabel_format(useOffset=False)
        ax.hist2d(x,y,bins=bins2d,norm=norm)

    plot_l(arrs, "l1", axes[1])
    plot_l(arrs, "l2", axes[2])

    plt.savefig(ofile)


def plot_dimp(wc, ifile, ofile):
    arrs = numpy.load(ifile)

    fig = plt.figure(figsize=(11,8), tight_layout=True)
    gs = GridSpec(1, 4, figure=fig)

    nax = 0
    axes = [
        fig.add_subplot(gs[nax, 0:2]),
        fig.add_subplot(gs[nax, 2:4]),
    ]

    stit = f'{wc.tier} ({wc.generator})'
    fig.suptitle(stit)

    alpha = 0.5

    for ax in axes:
        ax.set_yscale('log')

    axes[0].set_title('L1(a,b)')
    axes[0].hist(arrs['l1'])

    axes[1].set_title('L2(a,b)')
    axes[1].hist(arrs['l2'])

    plt.savefig(ofile)


def main_coal_file(coalfile, *evefiles):
    coalesce(evefiles, coalfile)


def main_run_pimp(rcoalfile, fcoalfile):
    wc = coal_tuple(fcoalfile)
    pimpfile = coal_aname(wc, origin='pimp')
    print (pimpfile)
    pimp(wc, rcoalfile, fcoalfile, pimpfile)
    return pimpfile
    
def main_plot_pimp(pimpfile, plotfile=None):
    wc = coal_tuple(pimpfile)
    plotfile = plotfile or pimpfile.replace(".npz", ".pdf")
    plot_pimp(wc, pimpfile, plotfile)
    return plotfile

def main_run_dimp(acoalfile, bcoalfile):
    wc = coal_tuple(acoalfile)
    dimpfile = coal_aname(wc, origin="dimp", domain="ab") 
    dimp(acoalfile, bcoalfile, dimpfile)
    return dimpfile

def main_plot_dimp(dimpfile, plotfile=None):
    wc = coal_tuple(dimpfile)
    plotfile = plotfile or dimpfile.replace(".npz", ".pdf")
    plot_dimp(wc, dimpfile, plotfile)
    return plotfile

def main_import_66(eve = 66):
    '''Deal with forgotten sample 66.

    It is shown in figure 7 of the original draft of the "data release" paper.

    Note, unlike the new 10x1000 sample simulation, the run of 32 and 99 samples
    had an electron sign error which we correct here as part of the special
    import.
    '''
    # paths on hierocles.phy.bnl.gov
    sbase="/srv/bviren/public_html/ls4gan/samples_sigproc/selected_translations_"
    dbase="/srv/bviren/public_html/ls4gan/toysp/data/samples/"
    def do_import(gen, dgen, dom, fr='fake', eve=66):
        src=f'{sbase}{gen}/{fr}_{dom}/sample_{eve}.npz'        
        dst=f'{dbase}{dgen}/{dom}/{eve}/raw.npz'
        Path(dst).parent.mkdir(exist_ok=True)

        # note, all "real" array dirs are links to UVCGAN's real_{a,b} and we
        # named "WCSim" arrays likewise.
        aname = generator_array_name[dgen]
        arr = numpy.load(src)[aname]
        print(gen,aname,arr.dtype,arr.shape, numpy.sum(arr), numpy.median(arr))
        arr = arr * -1.0        # we had a sign error in the original sim
        print(dgen,aname,arr.dtype,arr.shape, numpy.sum(arr), numpy.median(arr))
        arrays = {aname:arr}
        numpy.savez_compressed(dst, **arrays)
        print(f'{src}\n-->\n{dst}\n')
    for gen in generator_array_name:
        fr = "fake"
        dgen = gen
        if gen == "WCSim":
            fr = "real"
            gen = "UVCGAN"
        for dom in "ab":
            do_import(gen, dgen, dom, fr=fr, eve=eve)
            


if '__main__' == __name__:
    import sys

    meth = globals()["main_" + sys.argv[1]]
    got = meth(*sys.argv[2:])
    print(got)

    # coalesce(sys.argv[2:], sys.argv[1])
    # /home/bv/wrk/wct/point-cloud/toolkit/build/sigproc/test/adc-noise-sig/ACLGAN/pimp/a/sig.npz
    
