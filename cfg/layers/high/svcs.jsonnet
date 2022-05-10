// A number of "service" type components are shared by many components
// and each may have different implementations.  It is typically up to
// the "end user" to determine which implementation of these services
// to use and their configuration.  This Jsonnet helps to create a
// "service pack" from high-level parameters.  A "service pack" is
// simply an object which is keyed by canonical labels for a service
// such as "random" or "dft".

// There are a few DFT service implementations that are configured for
// specific "platforms"
local dfts = {
    default: self.cpu,
    cpu: { type: "FftwDFT" },
    cuda: { type: "CudaDFT" },
    torchcpu: { type: "TorchDFT", data: { device: "cpu" } },
    torchgpu: { type: "TorchDFT", data: { device: "gpu" } },
};

function(platform="default", seeds=[0,1,2,3,4], generator="default")
{
    // Fixme: may need to make the "platform" more fine grained.  At
    // least we have DFT and DNNROI which could use different
    // platforms.  For now, muddle on....
    platform: platform,

    dft: dfts[platform],

    // note in future, the IRandom may also change based on platform
    // if/when someone implement one based on cuda or torch or ... 
    random : {
        type: "Random",
        name: generator + "-" +
              std.join('-', std.map(std.toString,seeds)),
        data: {
            generator: generator,
            seeds: seeds,
        }
    },
}
