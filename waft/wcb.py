# Aggregate all the waftools to make the main wscript a bit shorter.
# Note, this is specific to WC building

from . import generic
import os.path as osp
from waflib.Utils import to_list

mydir = osp.dirname(__file__)

## These are packages descriptions which fit the generic functions.
## They will be checked in order so put any dependencies first.
package_descriptions = [

    # spdlog is "header only" but use library version for faster recompilation
    # wire-cell-util and ZIO both use this
    # Need to build with -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    ('spdlog',   dict(incs=['spdlog/spdlog.h'], libs=['spdlog'])),

    ('ZLib',     dict(incs=['zlib.h'], libs=['z'])),
    ('FFTW',     dict(incs=['fftw3.h'], libs=['fftw3f'], pcname='fftw3f')),
    ('FFTWThreads', dict(libs=['fftw3f_threads'], pcname='fftw3f', mandatory=False)),
    ('JsonCpp',  dict(incs=["json/json.h"], libs=['jsoncpp'])),

    ('Eigen',    dict(incs=["Eigen/Dense"], pcname='eigen3')),

    # for faster parsing, consider:
    # ./wcb configure --with-jsonnet-libs=gojsonnet 
    ('Jsonnet',  dict(incs=["libjsonnet.h"], libs=['jsonnet'])),
    ('TBB',      dict(incs=["tbb/parallel_for.h"], libs=['tbb'], mandatory=False)),
    ('HDF5',     dict(incs=["hdf5.h"], libs=['hdf5'], mandatory=False)),
    ('H5CPP',    dict(incs=["h5cpp/all"], mandatory=False, extuses=('HDF5',))),

    ('ZMQ',      dict(incs=["zmq.h"], libs=['zmq'], pcname='libzmq', mandatory=False)),
    ('CZMQ',     dict(incs=["czmq.h"], libs=['czmq'], pcname='libczmq', mandatory=False)),
    ('ZYRE',     dict(incs=["zyre.h"], libs=['zyre'], pcname='libzyre', mandatory=False)),
    ('ZIO',      dict(incs=["zio/node.hpp"], libs=['zio'], pcname='libzio', mandatory=False,
                      extuses=("ZYRE","CZMQ","ZMQ"))),

    # Note, this list may be modified (appended) in wscript files.
    # The list here represents the minimum wire-cell-toolkit requires.
]


def options(opt):

    # from here
    opt.load('boost')
    opt.load('smplpkgs')
    opt.load('rootsys')
    opt.load('libtorch')
    opt.load('cuda')
    opt.load('kokkos')
    #opt.load('protobuf')

    for name,desc in package_descriptions:
        generic._options(opt, name,
                         desc.get("incs", None),
                         desc.get("libs", None))

    opt.add_option('--build-debug', default='-O2 -ggdb3',
                   help="Build with debug symbols")

def find_submodules(ctx):
    sms = list()
    for wb in ctx.path.ant_glob("**/wscript_build"):
        sms.append(wb.parent.name)
    sms.sort()
    return sms


def configure(cfg):
    print ('Compile options: %s' % cfg.options.build_debug)

    cfg.load('boost')
    cfg.load('smplpkgs')

    for name, args in package_descriptions:
        #print ("Configure: %s %s" % (name, args))
        generic._configure(cfg, name, **args)
        #print ("configured %s" % name)

    if getattr(cfg.options, "with_libtorch", False) is False:
        print ("sans libtorch")
    else:
        cfg.load('libtorch')

    if getattr(cfg.options, "with_cuda", False) is False:
        print ("sans CUDA")
    else:
        cfg.load('cuda')

    if getattr(cfg.options, "with_kokkos", False) is False:
        print ("sans KOKKOS")
    else:
        cfg.load('kokkos')

    if getattr(cfg.options, "with_root", False) is False:
        print ("sans ROOT")
    else:
        cfg.load('rootsys')

    ### not yet used
    # if cfg.options.with_protobuf is False:
    #     print ("sans protobuf")
    # else:
    #     cfg.load('protobuf')


    def haveit(one):
        one=one.upper()
        if 'LIB_'+one in cfg.env:
            cfg.env['HAVE_'+one] = 1
            print('HAVE %s libs'%one)
        else:
            print('NO %s libs'%one)

    # Check for stuff not found in the wcb-generic way

    cfg.check_boost(lib='system filesystem graph thread program_options iostreams regex')
    haveit('boost')

    cfg.check(header_name="dlfcn.h", uselib_store='DYNAMO',
              lib=['dl'], mandatory=True)
    haveit('dynamo')

    cfg.check(features='cxx cxxprogram', lib=['pthread'], uselib_store='PTHREAD')
    haveit('pthread')


    cfg.env.CXXFLAGS += to_list(cfg.options.build_debug)
    cfg.env.CXXFLAGS += ['-DEIGEN_FFTW_DEFAULT=1']

    cfg.env.LIB += ['z']
    
    submodules = find_submodules(cfg)

    # submodules = 'util iface gen sigproc img pgraph apps sio dfp tbb ress cfg root'.split()
    # submodules.sort()
    # submodules = [sm for sm in submodules if osp.isdir(sm)]

    # Remove WCT packages if they an optional dependency wasn't found
    for pkg,ext in [
            ("root","HAVE_ROOTSYS"),
            ("tbb","HAVE_TBB"),
            ("tbb","LIB_FFTWTHREADS"),
            ("cuda","HAVE_CUDA"),
            ("hio", "INCLUDES_H5CPP"),
            ("pytorch", "LIB_LIBTORCH"),
            ("zio", "LIB_ZIO LIB_ZYRE LIB_CZMQ LIB_ZMQ")
    ]:
        exts = to_list(ext)
        for have in exts:
            if have in cfg.env or have in cfg.env.define_key:
                continue
            if pkg in submodules:
                print ('Removing package "%s" due to lack of external dependency "%s"'%(pkg,have))
                submodules.remove(pkg)

    cfg.env.SUBDIRS = submodules
    print ('Configured for submodules: %s' % (', '.join(submodules), ))
    cfg.write_config_header('config.h')
    #print(cfg.env)

def build(bld):
    bld.load('smplpkgs')

    subdirs = bld.env.SUBDIRS
    print ('Building: %s' % (', '.join(subdirs), ))
    bld.recurse(subdirs)

    if hasattr(bld, "smplpkg_graph"):
        # fixme: this writes directly.  Should make it a task, including
        # running graphviz to produce PNG/PDF
        print ("writing wct-depos.dot")
        bld.path.make_node("wct-deps.dot").write(str(bld.smplpkg_graph))


