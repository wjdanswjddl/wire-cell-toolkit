from waflib.Logs import debug, info, error, warn
import waflib.Utils

# Default versions of WCT test data to download t make available to
# historical tests.
TEST_DATA_VERSIONS='0.20.0,0.21.0,0.22.0,0.23.0,0.24.1'


def options(opt):
    # fixme: need to provide "official" URL.  For now:
    # https://www.phy.bnl.gov/~bviren/tmp/wcttest/data_repo
    opt.add_option('--test-data-url', type=str, default="https://www.phy.bnl.gov/~bviren/tmp/wcttest/data_repo",
                   help="URL for test data repository [default: see source]")
    opt.add_option('--test-data-versions', type=str, default=TEST_DATA_VERSIONS,
                   help="comma-separated list of tags for which historical test data files are downloaded [default: see source]")


def configure(cfg):

    # fixme: MUST load datarepo after wcb_unit_test
    # fixme: refactor datarepo, smplpkgs and wcb_unit_test

    versions = cfg.options.test_data_versions
    versions = versions.split(",")
    cfg.env.TEST_DATA_VERSIONS = versions

    url = cfg.options.test_data_url
    if not url:
        warn("No test data URL, test data repository will not be populated")
        return

    btnode = cfg.path.find_dir("build/tests")
    btnode.mkdir()
    btnode.make_node("input").mkdir()
    btnode.make_node("input.url").write(url + "/input.tar\n")

    for ver in versions:
        btnode.make_node("history-" + ver + ".url").write(url + "/history-" + ver + ".tar\n")
    

def populate_test_data(task):
    unode = task.inputs[0]
    lnode = task.outputs[0]

    cwd = unode.parent.abspath()
    url = unode.read().strip()
    lst = lnode.abspath()

    # fixme: replace with urllib + tarlib in a rule function?
    curl = "curl --silent " + url
    tar = "tar -C %s -xvf - > %s" % (cwd, lst)
    cmd = "bash -c '%s | %s'" % (curl, tar)
    info(cmd)
    return task.exec_command(cmd, shell=True)

    

def build(bld):
    '''
    Download and unpack archived data repo files.

    This relies on configure to create .url files.
    '''

    # fixme: MUST load datarepo after wcb_unit_test

    # don't bother if users has tests turned off
    if not bld.env.TESTS:
        info("test data repository will not be populated")
        return

    bld.cycle_group("datarepo")

    btnode = bld.bldnode.find_dir("tests")

    def make_dl_task(category, version=None):
        if version is None:
            unode = btnode.find_or_declare("input.url")
            lnode = btnode.find_or_declare("input.lst")
            name="test_data_input"
        else:
            base = "history-" + version
            unode = btnode.find_or_declare(base + ".url")
            lnode = btnode.find_or_declare(base + ".lst")
            name="test_data_history_" + version.replace(".","_")

        info(str(unode))
        info(str(lnode))
        bld(rule=populate_test_data, name=name,
            source=[unode], target=[lnode])
            
    make_dl_task("input")
    for ver in bld.env.TEST_DATA_VERSIONS:
        make_dl_task("history", ver)

    # url = bld.options.test_data_url
    # if not url:
    #     warn("Must provide --test-data-url=<url> to download test data repo")
    #     return -1
    
    # tstdir=bld.path.find_dir("build").make_node("tests")
    # tstdir.mkdir()

    # rurl="%s/input.tar" % (url, )
    # info(f"downloading {rurl}")
    # cmd = "curl --silent %s | tar -C %s -xf -" % (rurl, tstdir.abspath())
    # rc = waflib.Utils.subprocess.call(cmd, shell=True)
    # if rc != 0:
    #     warn("installation of input test data files failed")
    #     return -1
    
    # rels = bld.options.test_data_releases
    # if not rels:
    #     warn("Must provide --test-data-releases=<tag>,<tag>,...  to download historical data")
    #     return -1

    # rels = [r.strip() for r in rels.split(",") if r.strip()]
    # for rel in rels:
    #     rurl="%s/history-%s.tar" % ( url, rel )
    #     info(f"downloading {rurl}")
    #     cmd = "curl --silent %s | tar -C %s -xf -" % (rurl, tstdir.abspath())
    #     rc = waflib.Utils.subprocess.call(cmd, shell=True)
    #     if rc != 0:
    #         warn("installation of history test data files failed for version %s" % rel)
    #         return -1
        
def packrepo(bld):
    '''
    Pack existing test data repo to archive files.
    '''
    indir=bld.path.find_dir("build/tests/input")
    if indir.exists():
        cmd = "tar -C %s -cf input.tar input" % (indir.parent.abspath())
        info(cmd)
        rc = waflib.Utils.subprocess.call(cmd, shell=True)
        if rc != 0:
            warn("archive failed")
    else:
        warn("no input files found")


    
    rels = bld.options.test_data_releases
    if rels:
        rels = [r.strip() for r in rels.split(",") if r.strip()]

    hdir = bld.path.find_dir("build/tests/history")
    # print(hdir)
    hdirs = hdir.ant_glob("*")  # this finds children but doesn't return them?
    # print(hdirs)
    # hdir.mkdir()
    # print(hdir.children)
    for vdir in hdir.children:
        if rels and vdir not in rels:
            continue
        cmd = "tar -C %s -cf history-%s.tar history/%s" % (
            hdir.parent.abspath(), vdir, vdir)
        info(cmd)
        rc = waflib.Utils.subprocess.call(cmd, shell=True)
        if rc != 0:
            warn("archive failed")
        
