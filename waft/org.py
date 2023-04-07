import time
from waflib.Utils import to_list, subst_vars
from waflib.Task import Task, TaskSemaphore
from waflib.Logs import debug, info, error, warn
from waflib import TaskGen

class org_export(Task):

    export_file_name = None

    # Emacs will only write to a fixed file name, which then this task
    # may move to the target.  But, two tasks making a file (of the
    # same extension) will collide.  
    semaphore = TaskSemaphore(1)

    def scan(self):

        node = self.inputs[0]
        deps = list()
        for line in node.read().split("\n"):

            efn = "#+export_file_name:"
            if line.lower().startswith(efn):
                self.export_file_name = line.split(":",1)[1].strip()
                debug("org: scan found " + self.export_file_name)
                continue

            pre="#+include:"
            if line.lower().startswith(pre):
                fname = line[len(pre):].strip()
                debug("org: scan found " + fname)
                dep = node.parent.find_resource(fname)
                deps.append(dep)

        return (deps,[])

    def run(self):
        onode = self.inputs[0]
        enode = self.outputs[0]
        dotext = '.' + self.func.split("-")[-1]

        efn = self.export_file_name
        if efn:
            if not efn.endswith(dotext):
                efn += dotext
            tmp = onode.parent.make_node(efn)
        else:
            tmp = onode.abspath().replace('.org', dotext)

        debug(f"org: emacs will produce: {tmp}")

        cmd = "${EMACS} %s --batch -f %s" % (onode.abspath(), self.func)
        if tmp != enode.abspath():
            debug(f"org: will move {tmp} to {enode}")
            move = "mv %s %s" % (tmp, enode.abspath())
            cmd += " && " + move
        cmd = subst_vars(cmd, self.env)
        return self.exec_command(cmd, shell=True)
    

class org_export_html(org_export):
    func = "org-html-export-to-html"

class org_export_pdf(org_export):
    func = "org-latex-export-to-pdf"


@TaskGen.feature("org2html", "org2pdf", "inplace")
@TaskGen.before('process_source')
def transform_source(tgen):
    tgen.inputs = tgen.to_nodes(getattr(tgen, 'source', []))


@TaskGen.extension('.org')
def process_org(tgen, node):
    
    tgt = getattr(tgen, 'target', [])
    if isinstance(tgt, str):
        tgt = tgen.bld.path.find_or_declare(tgt)
    tgt = tgen.to_nodes(tgt)

    if 'org2html' in tgen.features:
        tgt = tgt or node.change_ext(".html")
        tsk = tgen.create_task("org_export_html", node,tgt)

    if 'org2pdf' in tgen.features:
        tgt = tgt or node.change_ext(".pdf")
        tsk = tgen.create_task("org_export_pdf", node, tgt)

    tsk.inplace = 'inplace' in tgen.features


def configure(cfg):
    cfg.find_program("emacs", var="EMACS")


def build(cfg):
    pass
