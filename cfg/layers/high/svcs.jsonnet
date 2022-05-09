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
    default: self.fftw,
    fftw: { type: "FftwDFT" },
    cudafft: { type: "CudaDFT" },
    torchcpu: { type: "TorchDFT", data: { device: "cpu" } },
    torchgpu: { type: "TorchDFT", data: { device: "gpu" } },
    "torchgpu0": { type: "TorchDFT", data: { device: "gpu0" } },
    "torchgpu1": { type: "TorchDFT", data: { device: "gpu1" } },
    "torchgpu2": { type: "TorchDFT", data: { device: "gpu2" } },
    "torchgpu3": { type: "TorchDFT", data: { device: "gpu3" } },
    // may need to extend this....
};

function(platform="default", seeds=[0,1,2,3,4], generator="default")
{
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
