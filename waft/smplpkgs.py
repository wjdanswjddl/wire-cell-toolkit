# -*- python -*-
'''This tool implements a source package following a few contentions.

Your source package may build any combination of the following:

 - shared libraries (src/*.cxx)
 - headers exposing an API to libraries (inc/NAME/*.h)
 - a ROOT dictionary for this API
 - main programs (apps/*.cxx)
 - test programs (test/test_*, test/check_*, wscript_build)

This tool will produce various methods on the build context.  You can
avoid passing <name> to them if you set APPNAME in your wscript file.

This file is part of the wire-cell-toolkit but we keep it free of
direct concepts specific to building WCT.  See wcb.py for those.

'''

import os
from contextlib import contextmanager
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
                extra = ""
                if cat == "tst":
                    extra=',constraint=false'
                lines.append('\t"%s" -> "%s"[color="%s"%s];' % \
                             (edge[0], edge[1], self.colors[cat], extra))
        lines.append('}')
        return '\n'.join(lines)

    def register(self, pkg, **kwds):
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
            
        

_tooldir = os.path.dirname(os.path.abspath(__file__))

def options(opt):
    opt.load('compiler_cxx')
    opt.load('waf_unit_test')
    opt.add_option('--nochecks', action='store_true', default=False,
                   help='Exec no checks', dest='no_checks')

def configure(cfg):
    cfg.load('compiler_cxx')
    cfg.load('waf_unit_test')
    cfg.load('rpathify')

    cfg.env.append_unique('CXXFLAGS',['-std=c++17'])

    # Do not add any things specific to WCT here.  see wcb.py instead.

    # interpreters
    cfg.find_program('python', var='PYTHON', mandatory=True)
    cfg.find_program('bash', var='BASH', mandatory=True)
    cfg.find_program('bats', var='BATS', mandatory=False)
    cfg.find_program('jsonnet', var='JSONNET', mandatory=False)

    # For testing
    cfg.find_program('diff', var='DIFF', mandatory=False)
    pass

def build(bld):
    from waflib.Tools import waf_unit_test
    bld.add_post_fun(waf_unit_test.summary)

@conf
def cycle_group(bld, gname):
    if gname in bld.group_names:
        bld.set_group(gname)
    else:
        bld.add_group(gname)


# from waflib import Task, TaskGen
# @TaskGen.feature('test_variant')
# @TaskGen.after_method('process_source', 'apply_link')
# def make_test_variant(self):
#     cmdline = self.ut_str
#     progname, argline = cmdline.split(' ',1)

#     tvp = 'TEST_VARIANT_PROGRAM'

#     output = self.path.find_or_declare(self.name + ".passed")

#     source = getattr(self, 'source', None)
#     srcnodes = self.to_nodes(source)

#     if not source and progname.startswith("${"):
#         warn("parameterized program name with lacking inputs is not supported: " + cmdline)
#         return

#     if not progname.startswith("${"):
#         prognode = self.path.find_or_declare(progname)
#         progname = "${%s}" % tvp

#     cmdline = "%s %s && touch %s" % (progname, argline, output.abspath())

#     tsk = self.create_task('utest', srcnodes, [output])

#     if tvp in progname:
#         tsk.env[tvp] = prognode.abspath()
#         tsk.vars.append(tvp)
#         tsk.dep_nodes.append(prognode)

#     self.ut_run, lst = Task.compile_fun(cmdline, shell=True)
#     tsk.vars = lst + tsk.vars

#     if getattr(self, 'ut_cwd', None):
#         self.handle_ut_cwd('ut_cwd')
#     else:
#         self.ut_cwd = self.path

#     if not self.ut_cwd.exists():
#         self.ut_cwd.mkdir()

#     if not hasattr(self, 'ut_env'):
#         self.ut_env = dict(os.environ)
#         def add_paths(var, lst):
#             # Add list of paths to a variable, lst can contain strings or nodes
#             lst = [ str(n) for n in lst ]
#             debug("ut: %s: Adding paths %s=%s", self, var, lst)
#             self.ut_env[var] = os.pathsep.join(lst) + os.pathsep + self.ut_env.get(var, '')


# A "fake" waf context (but real Python context manager) which will
# return the Waf "group" to "libraries" on exit and which provides
# several task generators and which defines tasks for the default
# convention of test/test_*.* and test/check_*.cxx.  This is returned
# by smplpkg().
class ValidationContext:

    compiled_extensions = ['.cxx', '.kokkos']
    script_interpreters = {'.py':"python", '.bats':'bats', '.sh':'bash', '.jsonnet':'jsonnet'}

    def __init__(self, bld, uses):
        '''
        Uses must include list of dependencies in "uses".
        '''
        self.bld = bld
        self.uses = to_list(uses)

        if self.bld.options.no_tests:
            self.bld.options.no_checks = True # need tests for checks, in general
            info("atomic unit tests will not be built nor run for " + self.bld.path.name)
            return

        self.bld.cycle_group("validations")

        # Fake out: checks need tests but tests do not need checks but
        # tests and checks are defined by the same functions which
        # honor no_checks.
        no_checks = self.bld.options.no_checks
        self.bld.options.no_checks = False

        # Default patterns
        for one in self.bld.path.ant_glob('test/check_*.cxx'):
            self.program(one)

        # Atomic unit tests
        for ext in self.compiled_extensions:
            for one in self.bld.path.ant_glob('test/test_*'+ext):
                self.program(one, "test")

        for ext in self.script_interpreters:
            for one in self.bld.path.ant_glob('test/test*'+ext):
                self.script(one)

        self.bld.options.no_checks = no_checks

    def __enter__(self):
        if self.bld.options.no_checks:
            info("variant checks will not be built nor run for " + self.bld.path.name)
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        self.bld.cycle_group("libraries")
        return

    def nodify_resource(self, name_or_node, path=None):
        'Return a resource node'

        if path is None:
            path = self.bld.path
        if isinstance(name_or_node, waflib.Node.Node):
            return name_or_node
        return path.find_resource(name_or_node)

    def nodify_declare(self, name_or_node, path=None):
        'Return a resource node'

        if path is None:
            path = self.bld.path
        if isinstance(name_or_node, waflib.Node.Node):
            return name_or_node
        return path.find_or_declare(name_or_node)

    def program(self, source, features=""):
        '''Compile a C++ program to use in validation.

        Add "test" as a feature to also run as a unit test.

        '''
        if self.bld.options.no_checks:
            return
        features = ["cxx","cxxprogram"] + to_list(features)
        rpath = self.bld.get_rpath(self.uses) # fixme
        source = self.nodify_resource(source)
        ext = source.suffix()
        self.bld.program(source = [source], 
                         target = source.name.replace(ext,''),
                         features = features,
                         install_path = None,
                         #rpath = rpath,
                         includes = ['inc','test','tests'],
                         use = self.uses)

    def script(self, source):
        'Create a task for atomic unit test with an interpreted script'
        if self.bld.options.no_checks:
            return
        source = self.nodify_resource(source)
        ext = source.suffix()

        interp = self.script_interpreters.get(ext, None)

        if interp is None:
            warn(f'skipping script with no known interpreter: {source}')
            return

        INTERP = interp.upper()
        if INTERP not in self.bld.env:
            warn(f'skipping script with no found interpreter: {source}')
            return

        # info(f'{interp} {source}')
        self.bld(features="test_scripts",
                 ut_cwd   = self.bld.path, 
                 use = self.uses, 
                 test_scripts_source = source,
                 test_scripts_template = "${%s} ${SCRIPT}" % INTERP)

        
    # def variant(self, cmdline, **kwds):
    #     name = cmdline.replace(" ","_").replace("/","_").replace("$","_").replace("{","_").replace("}","_")
    #     self.bld(name=name, features="test_variant", ut_str = cmdline)


    def rule(self, rule, source="", target="", **kwds):
        'Simple wrapper for arbitrary rule'
        if self.bld.options.no_checks:
            return
        self.bld(rule=rule, source=source, target=target, **kwds)


    def rule_http_get(self, task):
        'A rule function transform a URL file into its target via HTTP(s)'
        if self.bld.options.no_checks:
            return
        from urllib.request import urlopen
        unode = task.inputs[0]
        remote = unode.read()
        text = urlopen(remote).read().decode()
        onode = task.outputs[0]
        onode.write(text)


    def rule_scp_get(self, task):
        'A rule function to transform a URL file its target via scp'
        if self.bld.options.no_checks:
            return
        unode = task.inputs[0]
        remote = unode.read()[4:]
        local = task.outputs[0].abspath();
        return task.exec_command(f'scp {remote} {local}')


    # def rule_cp_get(self, task):
    #     'A rule function to copy input to output'
    #     remote = task.inputs[0].abspath()
    #     local = task.outputs[0].abspath()
    #     return task.exec_command(f'cp {remote} {local}')

        
    def get_file(self, remote='...', local='...'):
        'Make task to bring remote file to local file'
        if self.bld.options.no_checks:
            return
        if remote.startswith(("http://","https://")):
            rule_get = lambda task: self.rule_http_get(task)
        elif remote.startswith("scp:"):
            rule_get = lambda task: self.rule_scp_get(task)
        else:
            raise ValueError("get file from absolute path not allowed")

        lnode = self.bld.path.find_or_declare(local)
        unode = lnode.parent.make_node(lnode.name + ".url")
        unode.write(remote)
        # make task instead of doing this immediately
        self.bld(rule=rule_get, source = unode, target = lnode)

    def put_file(self, local='...', remote='...'):
        if self.bld.options.no_checks:
            return
        raise Unimplemented()

    def diff(self, one, two):
        'Make a task to output a diff between two files'
        if self.bld.options.no_checks:
            return
        one = self.nodify_declare(one)
        two = self.nodify_declare(two)
        dnode = one.parent.find_or_declare(one.name +"_"+ two.name +".diff")
        self.bld(rule="${DIFF} ${SRC} > ${TGT}",
                 source=[one, two], target=[dnode], shell=True)
    

@conf
def get_rpath(bld, uselst, local=True):
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
                ret.add(pkgdir)
    ret = list(ret)
    ret.sort()
    return ret

@conf
def smplpkg(bld, name, use='', app_use='', test_use=''):

    use = list(set(to_list(use)))
    use.sort()
    app_use = list(set(use + to_list(app_use)))
    app_use.sort()
    test_use = list(set(use + to_list(test_use)))
    test_use.sort()

    if not hasattr(bld, 'smplpkg_graph'):
        bld.smplpkg_graph = SimpleGraph()
    bld.smplpkg_graph.register(
        name,
        lib=set(to_list(use)),
        app=set(to_list(app_use)),
        tst=set(to_list(test_use)))

    includes = [bld.out_dir]
    headers = []
    source = []

    incdir = bld.path.find_dir('inc')
    srcdir = bld.path.find_dir('src')
    dictdir = bld.path.find_dir('dict')
    appsdir = bld.path.find_dir('apps')

    bld.cycle_group("libraries")

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
        source += pbs

    # the library
    if srcdir:
        ei = ''
        if incdir:
            ei = 'inc' 
        bld(features = 'cxx cxxshlib',
            name = name,
            source = source,
            target = name,
            #rpath = bld.get_rpath(use),
            includes = includes, # 'inc',
            export_includes = ei,
            use = use)            

    bld.cycle_group("applications")

    if appsdir:
        for app in appsdir.ant_glob('*.cxx'):
            appbin = bld.path.find_or_declare(app.name.replace('.cxx',''))
            bld.program(source = [app], 
                        target = appbin,
                        includes =  includes, # 'inc',
                        #rpath = bld.get_rpath(app_use + [name], local=False),
                        use = app_use + [name])
            keyname = appbin.name.upper().replace("-","_")
            bld.env[keyname] = appbin.abspath()
                          
    bld.cycle_group("libraries")

    return ValidationContext(bld, test_use + [name])

