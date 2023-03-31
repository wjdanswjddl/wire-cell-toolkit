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
from waflib.Utils import to_list, subst_vars
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
    opt.load('wcb_unit_test') # adds --tests


def configure(cfg):
    cfg.load('compiler_cxx')
    cfg.load('wcb_unit_test')
    cfg.load('rpathify')

    cfg.env.append_unique('CXXFLAGS',['-std=c++17'])

    # Do not add any things specific to WCT here.  see wcb.py instead.

    # interpreters
    cfg.find_program('python', var='PYTHON', mandatory=True)
    cfg.find_program('bash', var='BASH', mandatory=True)
    cfg.find_program('bats', var='BATS', mandatory=False,
                     path_list=[os.path.realpath("test/bats/bin"),"/usr/bin","/usr/local/bin"])
    cfg.find_program('jsonnet', var='JSONNET', mandatory=False)

    # For testing
    cfg.find_program('diff', var='DIFF', mandatory=True)


def build(bld):
    bld.load('wcb_unit_test')


from wcb_unit_test import test_group_sequence

@conf
def cycle_group(bld, gname):
    if gname in bld.group_names:
        bld.set_group(gname)
    else:
        bld.add_group(gname)

# A "fake" waf context (but real Python context manager) which will
# return the Waf "group" to "libraries" on exit and which provides
# several task generators and which defines tasks for the default
# convention of test/test_*.* and test/check_*.cxx.  This is returned
# by smplpkg().
class ValidationContext:

    compiled_extensions = ['.cxx', '.kokkos']

    # Script CLI templates by file extension.
    # Must include command and script variables.
    script_templates = {
        '.py': "${PYTHON} ${SCRIPT}",
        '.pyt':"${PYTEST} ${SCRIPT}",
        '.sh':'${BASH} ${SCRIPT}',
        # Explicitly force bats to run serially
        '.bats':'${BATS} --jobs 1 ${SCRIPT}',
        '.jsonnet':'${JSONNET} ${SCRIPT}'}

    extra_prefixes = dict(atomic=("test",))

    def __init__(self, bld, uses):
        '''
        Uses must include list of dependencies in "uses".
        '''
        self.bld = bld
        self.uses = to_list(uses)

        if not self.bld.env.TESTS:
            debug("smplpkgs: tests suppressed for " + self.bld.path.name)
            return

        # We impose a series of groups to assure coarse grained
        # dependency with a source naming convention to separate
        # different intentions.  Map scope name to source file name
        # prefix.
        extra_prefixes = dict(atomic=("test",))

        match_pat = 'test/%s*%s' #  (prefix, ext)

        for group in test_group_sequence:
            self.do_group(group)
            if group == bld.env.TEST_GROUP:
                break

    def do_group(self, group):
        self.bld.cycle_group("testing_"+group)
        features = "" if group == "check" else "test"

        prefixes = [group]
        prefixes += self.extra_prefixes.get(group, [])
        match_pat = 'test/%s*%s' #  (prefix, ext)

        for prefix in prefixes:

            for ext in self.compiled_extensions:
                pat = match_pat % (prefix, ext)
                for one in self.bld.path.ant_glob(match_pat % (prefix, ext)):
                    debug("tests: (%s) %s" %(features, one))
                    self.program(one, features)

            if group == "check":
                continue

            for ext in self.script_templates:
                for prefix in prefixes:
                    for one in self.bld.path.ant_glob(match_pat % (prefix, ext)):
                        self.script(one)
        


    def __enter__(self):
        if not self.bld.env.TESTS:
            debug("smplpkgs: variant checks will not be built nor run for " + self.bld.path.name)
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        self.bld.cycle_group("libraries")
        return

    def has(self, *args):
        '''
        Return true if we are built with all pkgs 
        '''
        if not args:
            return true
        
        pkgs = list()
        for a in args:
            pkgs += to_list(a)

        have_all = True
        for pkg in pkgs:
            maybe = 'HAVE_' + pkg.upper()
            if not maybe in self.bld.env:
                have_all = False
        return have_all

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
        if not self.bld.env.TESTS:
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
        if not self.bld.env.TESTS:
            return
        source = self.nodify_resource(source)
        ext = source.suffix()
        name = source.name.replace(ext,'')
        outnode = self.bld.path.find_or_declare(name + '.out')

        templ = self.script_templates.get(ext, None)

        if templ is None:
            warn(f'skipping script of unsupported extention: {ext}')
            return

        cli = subst_vars(templ, self.bld.env)
        interp = cli.split(" ")[0].strip()
        if not interp:
            warn(f'skipping script with no found interpreter: {source}')
            return

        # debug('script: ' + cli)
        self.bld(features="test_scripts",
                 name=name + '_' + os.path.basename(interp),
                 ut_cwd   = self.bld.path, 
                 use = self.uses, 
                 test_scripts_source = source,
                 test_scripts_template = templ,
                 target = [outnode])

        
    # def variant(self, cmdline, **kwds):
    #     name = cmdline.replace(" ","_").replace("/","_").replace("$","_").replace("{","_").replace("}","_")
    #     self.bld(name=name, features="test_variant", ut_str = cmdline)


    def rule(self, rule, source="", target="", **kwds):
        'Simple wrapper for arbitrary rule'
        if not self.bld.env.TESTS:
            return
        self.bld(rule=rule, source=source, target=target, **kwds)


    def rule_http_get(self, task):
        'A rule function transform a URL file into its target via HTTP(s)'
        if not self.bld.env.TESTS:
            return
        from urllib.request import urlopen
        unode = task.inputs[0]
        remote = unode.read()
        text = urlopen(remote).read().decode()
        onode = task.outputs[0]
        onode.write(text)


    def rule_scp_get(self, task):
        'A rule function to transform a URL file its target via scp'
        if not self.bld.env.TESTS:
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
        if not self.bld.env.TESTS:
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
        if not self.bld.env.TESTS:
            return
        raise Unimplemented()

    def diff(self, one, two):
        'Make a task to output a diff between two files'
        if not self.bld.env.TESTS:
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
    docdir = bld.path.find_dir('docs')

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

    # hack in to the env entries for the apps we build
    validation_envs = dict()

    if appsdir:
        for app in appsdir.ant_glob('*.cxx'):
            appbin = bld.path.find_or_declare(app.name.replace('.cxx',''))
            bld.program(source = [app], 
                        target = appbin,
                        includes =  includes, # 'inc',
                        #rpath = bld.get_rpath(app_use + [name], local=False),
                        use = app_use + [name])
            keyname = appbin.name.upper().replace("-","_")
            validation_envs[keyname] = appbin.abspath()
                          
    bld.cycle_group("documentation")
    if docdir and "PANDOC" in bld.env:
        for docsrc in docdir.ant_glob("*.org"):
            docname = docsrc.name.replace('.org','')
            if docname.startswith("setup"):
                continue
            docpdf = bld.path.find_or_declare(docname + ".pdf")
            bld(rule="pandoc -o ${TGT[0].abspath()} ${SRC[0].abspath()}",
                source = [docsrc], target = [docpdf])

    bld.cycle_group("validations")
    for key,val in validation_envs.items():
        bld.env[key] = val

    bld.cycle_group("libraries")

    # Return even if we are no tests so as to not break code in
    # wscript_build that uses this return to define "variant" tests.
    return ValidationContext(bld, test_use + [name])

