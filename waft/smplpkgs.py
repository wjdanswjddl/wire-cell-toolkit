# -*- python -*-
'''This tool implements a source package following a few contentions.

Your source package may build any combination of the following:

 - shared libraries 
 - headers exposing an API to libraries
 - a ROOT dictionary for this API
 - main programs
 - test programs

This tool will produce various methods on the build context.  You can
avoid passing <name> to them if you set APPNAME in your wscript file.

'''

import os.path as osp
from waflib.Utils import to_list
from waflib.Configure import conf
import waflib.Context
from waflib.Logs import debug, info, error, warn

class SimpleGraph(object):
    colors = dict(lib='black', app='blue', tst='gray')
    def __init__(self):
        self._nodes = dict()
        self._edges = dict()

    def __str__(self):
        lines = ['digraph deps {']
        for node,attrs in self._nodes.items():
            lines.append('\t"%s";' % node)

        for edge, attrs in self._edges.items():
            for cat in attrs:
                # print (edge, cat, self.colors[cat])
                extra = ""
                if cat == "tst":
                    extra=',constraint=false'
                lines.append('\t"%s" -> "%s"[color="%s"%s];' % \
                             (edge[0], edge[1], self.colors[cat], extra))
        lines.append('}')
        return '\n'.join(lines)

    def register(self, pkg, **kwds):
        # print ("register %s" % pkg)
        self.add_node(pkg)
        for cat, deps in kwds.items():
            kwds = {cat: True}
            for other in deps:
                self.add_edge((pkg, other), **kwds)
        
    def add_node(self, name, **kwds):
        if name in self._nodes:
            self._nodes[name].update(kwds)
        else:
            self._nodes[name] = kwds
    def add_edge(self, edge, **kwds):
        if edge in self._edges:
            self._edges[edge].update(kwds)
        else:
            self._edges[edge] = kwds
            
        

_tooldir = osp.dirname(osp.abspath(__file__))

def options(opt):
    opt.load('compiler_cxx')
    opt.load('waf_unit_test')
    

def configure(cfg):
    cfg.load('compiler_cxx')
    cfg.load('waf_unit_test')
    cfg.load('rpathify')

    cfg.env.append_unique('CXXFLAGS',['-std=c++17'])

    cfg.find_program('python', var='PYTHON', mandatory=True)
    cfg.find_program('bash', var='BASH', mandatory=True)
    pass

def build(bld):
    from waflib.Tools import waf_unit_test
    bld.add_post_fun(waf_unit_test.summary)
    #print ("smplpkgs.build()")

@conf
def smplpkg(bld, name, use='', app_use='', test_use=''):

    if not hasattr(bld, 'smplpkg_graph'):
        #print ("Make SimpleGraph")
        bld.smplpkg_graph = SimpleGraph()
    bld.smplpkg_graph.register(
        name,
        lib=set(to_list(use)),
        app=set(to_list(app_use)),
        tst=set(to_list(test_use)))

    use = list(set(to_list(use)))
    use.sort()
    app_use = list(set(use + to_list(app_use)))
    app_use.sort()
    test_use = list(set(use + to_list(test_use)))
    test_use.sort()

    includes = [bld.out_dir]
    headers = []
    source = []

    incdir = bld.path.find_dir('inc')
    srcdir = bld.path.find_dir('src')
    dictdir = bld.path.find_dir('dict')

    testsrc = bld.path.ant_glob('test/test_*.cxx')
    testsrc_kokkos = bld.path.ant_glob('test/test_*.kokkos')
    test_scripts = bld.path.ant_glob('test/test_*.sh') + bld.path.ant_glob('test/test_*.py')
    test_jsonnets = bld.path.ant_glob('test/test*.jsonnet')

    checksrc = bld.path.ant_glob('test/check_*.cxx')

    appsdir = bld.path.find_dir('apps')

    if incdir:
        headers += incdir.ant_glob(name + '/*.h')
        includes += ['inc']
        bld.env['INCLUDES_'+name] = [incdir.abspath()]

    if headers:
        bld.install_files('${PREFIX}/include/%s' % name, headers)

    if srcdir:
        source += srcdir.ant_glob('*.cxx')
        source += srcdir.ant_glob('*.cu') # cuda
        source += srcdir.ant_glob('*.kokkos') # kokkos

    # fixme: I should move this out of here.
    # root dictionary
    if dictdir:
        if not headers:
            error('No header files for ROOT dictionary "%s"' % name)
        #print 'Building ROOT dictionary: %s using %s' % (name,use)
        if 'ROOTSYS' in use:
            linkdef = dictdir.find_resource('LinkDef.h')
            bld.gen_rootcling_dict(name, linkdef,
                                   headers = headers,
                                   includes = includes, 
                                   use = use)
            source.append(bld.path.find_or_declare(name+'Dict.cxx'))
        else:
            warn('No ROOT dictionary will be generated for "%s" unless "ROOTSYS" added to "use"' % name)

    if hasattr(bld.env, "PROTOC"):
        pbs = bld.path.ant_glob('src/**/*.proto')
        # if ("zpb" in name.lower()):
        #     print ("protobufs: %s" % (pbs,))
        source += pbs

    def get_rpath(uselst, local=True):
        ret = set([bld.env["PREFIX"]+"/lib"])
        for one in uselst:
            libpath = bld.env["LIBPATH_"+one]
            for l in libpath:
                ret.add(l)
            if local:
                if one.startswith("WireCell"):
                    sd = one[8:].lower()
                    blddir = bld.path.find_or_declare(bld.out_dir)
                    pkgdir = blddir.find_or_declare(sd).abspath()
                    #print pkgdir
                    ret.add(pkgdir)
        ret = list(ret)
        ret.sort()
        return ret

    # the library
    if srcdir:
        if ("zpb" in name.lower()):
            print ("Building library: %s, using %s, source: %s"%(name, use, source))
        ei = ''
        if incdir:
            ei = 'inc' 
        bld(features = 'cxx cxxshlib',
            name = name,
            source = source,
            target = name,
            #rpath = get_rpath(use),
            includes = includes, # 'inc',
            export_includes = ei,
            use = use)            

    if appsdir:
        for app in appsdir.ant_glob('*.cxx'):
            #print 'Building %s app: %s using %s' % (name, app, app_use)
            bld.program(source = [app], 
                        target = app.name.replace('.cxx',''),
                        includes =  includes, # 'inc',
                        #rpath = get_rpath(app_use + [name], local=False),
                        use = app_use + [name])


    if (testsrc or testsrc_kokkos or test_scripts) and not bld.options.no_tests:
        for test_main in testsrc_kokkos:
            #print 'Building %s test: %s' % (name, test_main)
            rpath = get_rpath(test_use + [name])
            #print rpath
            bld.program(features = 'cxx cxxprogram test', 
                        source = [test_main], 
                        ut_cwd   = bld.path, 
                        target = test_main.name.replace('.kokkos',''),
                        install_path = None,
                        #rpath = rpath,
                        includes = ['inc','test','tests'],
                        use = test_use + [name])
        for test_main in testsrc:
            #print 'Building %s test: %s' % (name, test_main)
            rpath = get_rpath(test_use + [name])
            #print rpath
            bld.program(features = 'test', 
                        source = [test_main], 
                        ut_cwd   = bld.path, 
                        target = test_main.name.replace('.cxx',''),
                        install_path = None,
                        #rpath = rpath,
                        includes = ['inc','test','tests'],
                        use = test_use + [name])
        for test_script in test_scripts:
            interp = "${BASH}"
            if test_script.abspath().endswith(".py"):
                interp = "${PYTHON}"
            #print 'Building %s test %s script: %s using %s' % (name, interp, test_script, test_use)
            bld(features="test_scripts",
                ut_cwd   = bld.path, 
                test_scripts_source = test_script,
                test_scripts_template = "pwd && " + interp + " ${SCRIPT}")

    if test_jsonnets and not bld.options.no_tests:
        # print ("testing %d jsonnets in %s" % (len(test_jsonnets), bld.path ))
        for test_jsonnet in test_jsonnets:
            bld(features="test_scripts",
                ut_cwd   = bld.path, 
                test_scripts_source = test_jsonnet,
                test_scripts_template = "pwd && ../build/apps/wcsonnet ${SCRIPT}")

    if checksrc and not bld.options.no_tests:
        for check_main in checksrc:
            #print 'Building %s check: %s' % (name, check_main)
            rpath = get_rpath(test_use + [name])
            #print rpath
            bld.program(source = [check_main], 
                        target = check_main.name.replace('.cxx',''),
                        install_path = None,
                        #rpath = rpath,
                        includes = ['inc','test','tests'],
                        use = test_use + [name])
            
