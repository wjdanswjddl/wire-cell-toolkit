import generic

def options(opt):
    generic._options(opt, "libtorch")

def configure(cfg):
    # warning, list of libs changes over version.

    libs = getattr(cfg.options, "with_libtorch_libs", None)
    if not libs:
        libs = ['torch', 'torch_cpu', 'torch_cuda', 'c10_cuda', 'c10']

    generic._configure(cfg, "libtorch", 
                       incs=["torch/script.h"],
                       libs=libs,
                       mandatory=False)
    if 'torch_cuda' in libs:
        setattr(cfg.env, 'LINKFLAGS_LIBTORCH',
                ['-Wl,--no-as-needed,-ltorch_cuda','-Wl,--as-needed'])
