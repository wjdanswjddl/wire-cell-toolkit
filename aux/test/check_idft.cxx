/** Run IDFT operations on input array, write result

    Usage:

    check_idft -c cfg.[json|jsonnet] -o out.tar.bz2 [-p pi -t tn] in.tar.bz2 ...

    The configuration file should be a sequence of objects describing
    transformations.  Each object with these keys:

    - src : source array name 
    - dst : destination array name
    - op : operator name, see below.

    The operator is given as a keyword which encodes 

    - DIR : the direction of the transform, one in {fwd, inv}
    - R : the rank of the transform (not array), on in {1, 2}
    - V : a variant of "batched" ("b") or full dimensional ("d")
    - A : an optional "axis" number in {0,1} for bached
    - C : an optional conversion between real and complex, in {"r2c", "c2r"}

    Some examples:

    All complex input and complex output:

    - fwd1d :: 1D forward transform on a 1D array
    - inv2d :: 2D transform on a 2D array
    - fwd1b1 :: 1D forward transform on axis=1 (each row is transformed) on a 2D array

    Mixed real/complex

    - fwd1d_r2c :: fwd1d on 1D real array producing complex array
    - inv1d_c2r :: inv1d on 2D complex array producing real array

 */


#include "aux_test_dft_helpers.h"
#include "WireCellUtil/Stream.h"
#include "WireCellAux/DftTools.h"

#include <boost/iostreams/filtering_stream.hpp>

#include <map>
#include <string>
#include <complex>
#include <algorithm>

using namespace WireCell;
using namespace WireCell::Stream;
using namespace WireCell::Aux::Test;
using namespace WireCell::Aux::DftTools;

using scalar_t = float;
using array_xxf = Eigen::Array<scalar_t, Eigen::Dynamic, Eigen::Dynamic>;
using complex_t = std::complex<scalar_t>;
using array_xxc = Eigen::Array<complex_t, Eigen::Dynamic, Eigen::Dynamic>;

// may hold any dtype and shape
using pig_array = pigenc::File;
using array_store = std::map<std::string, pig_array>;

using dft_op = std::function<pig_array(pig_array)>;
using op_lu_t = std::map<std::string, dft_op>;

using vector_xf = std::vector<scalar_t>;
using vector_xc = std::vector<complex_t>;

template<typename Scalar>
static std::vector<Scalar> p2v(const pig_array& pa)
{
    if (pa.header().shape().size() != 1) {
        throw std::runtime_error("p2v rank mismatch");
    }
    auto vec = pa.as_vec<Scalar>();
    if (vec.empty()) {
        throw std::runtime_error("p2v type mismatch");
    }
    return vec;
}
template<typename Scalar>
pig_array v2p(const std::vector<Scalar>& vec)
{
    std::vector<char> data((const char*)vec.data(),
                           (const char*)vec.data() + sizeof(Scalar)*vec.size());
    pig_array pa;
    pa.set<Scalar>(data, {vec.size()});
    return pa;
}


template<typename Scalar>
Eigen::Array<Scalar, Eigen::Dynamic, Eigen::Dynamic> p2a(const pig_array& pa)
{
    if (pa.header().shape().size() != 2) {
        throw std::runtime_error("p2a rank mismatch");
    }
    Eigen::Array<Scalar, Eigen::Dynamic, Eigen::Dynamic> arr;
    bool ok = pigenc::eigen::load(pa, arr);
    if (!ok) {
        throw std::runtime_error("p2a type mismatch");
    }
    return arr;
}
template<typename Scalar>
pig_array a2p(const Eigen::Array<Scalar, Eigen::Dynamic, Eigen::Dynamic>& arr)
{
    pig_array pa;
    pigenc::eigen::dump(pa, arr);
    return pa;
}



pig_array dispatch(const IDFT::pointer& dft, const pig_array& pa, const std::string& op)
{
    // vector

    if (op == "fwd1d") 
        return v2p<complex_t>(fwd(dft, p2v<complex_t>(pa)));

    if (op == "inv1d") 
        return v2p<complex_t>(inv(dft, p2v<complex_t>(pa)));
            
    if (op == "fwd1d_r2c") 
        return v2p<complex_t>(fwd_r2c(dft, p2v<scalar_t>(pa)));

    if (op == "inv1d_c2r") 
        return v2p<scalar_t>(inv_c2r(dft, p2v<complex_t>(pa)));

    // array

    if (op == "fwd2d")
        return a2p<complex_t>(fwd(dft, p2a<complex_t>(pa)));

    if (op == "inv2d")
        return a2p<complex_t>(inv(dft, p2a<complex_t>(pa)));

    // if (op == "fwd2d_r2c")
    //     return a2p<complex_t>(fwd_r2c(dft, p2a<scalar_t>(pa)));

    // if (op == "inv2d_c2r")
    //     return a2p<scalar_t>(inv_c2r(dft, p2a<complex_t>(pa)));

    if (op == "fwd1b0")
        return a2p<complex_t>(fwd(dft, p2a<complex_t>(pa), 0));

    if (op == "fwd1b1")
        return a2p<complex_t>(fwd(dft, p2a<complex_t>(pa), 1));

    if (op == "inv1b0")
        return a2p<complex_t>(inv(dft, p2a<complex_t>(pa), 0));

    if (op == "inv1b1")
        return a2p<complex_t>(inv(dft, p2a<complex_t>(pa), 1));

    if (op == "" or op == "noop" or op == "no-op") {
        return pa;
    }

    throw std::runtime_error("unsupported op: " + op);
}

int main(int argc, char* argv[])
{
    DftArgs args;
    int rc = make_dft_args(args, argc, argv);
    if (rc) { return rc; }

    if (args.positional.empty()) {
        std::cerr << "need at least one input file" << std::endl;
        return 0;
    }
    if (args.output.empty()) {
        std::cerr << "need output file" << std::endl;
        return 0;
    }
    if (args.cfg.empty()) {
        std::cerr << "need configuration" << std::endl;
        return 0;
    }
    std::cerr << args.cfg << std::endl;
    
    auto idft = make_dft(args.tn, args.pi, args.cfg);

    array_store arrs;

    // Slurp in arrays.
    for (const auto& sname : args.positional) {
        boost::iostreams::filtering_istream ins;
        std::cerr << "openning: "<<sname<<"\n";

        input_filters(ins, sname);
        if (ins.size() < 2) {     // expect bz2 + tar filters + file source.
            std::cerr << "Unexpected file format with: "<<sname<<"\n";
            return 1;
        }
            
        while (true) {
            std::string fname{""};
            size_t fsize{0};
            custard::read(ins, fname, fsize);
            if (ins.eof()) {
                break;
            }
            if (!ins) {
                std::cerr << "ERROR unpacking tar header " << strerror(errno) << std::endl;
                return -1;
            }

            pigenc::File pig;
            pig.read(ins);
            if (!ins) {
                std::cerr << "ERROR unpacking pig " << strerror(errno) << std::endl;
                return -1;
            }
            auto npy = fname.find(".npy");
            if (npy != std::string::npos) {
                fname = fname.substr(0, npy);
            }
            std::cerr << "\tread " << fname
                      << " with dtype=" << pig.header().dtype()
                      << " shape: (";
            for (auto dim : pig.header().shape()) {
                std::cerr << " " << dim;
            }
            std::cerr << " )" << std::endl;

            arrs[fname] = pig;
        }
    }

    boost::iostreams::filtering_ostream outs;
    output_filters(outs, args.output);
    if (outs.size() < 2) {     // must have at least get tar filter + file sink.
        std::cerr << "Unexpected file format: " << args.output << "\n";
    }

    for (auto one : args.cfg) {
        auto src = one["src"].asString();
        auto it = arrs.find(src);
        if (it == arrs.end()) {
            std::cerr << "no src array: " << src << std::endl;
            continue;
        }

        auto op = one["op"].asString();
        auto dst = one["dst"].asString();
        std::cerr << op << "(" << src << ") -> " << dst << std::endl;
        auto darr = dispatch(idft, it->second, op);

        auto siz = darr.header().array_size();
        if (siz == 0) {
            std::cerr << "failed: " << op <<  "(" << src << ") -> " << dst << " (zero size)\n";
            continue;
        }


        auto fsiz = darr.header().file_size();
        auto npy = dst.find(".npy");
        if (npy == std::string::npos) {
            dst = dst + ".npy";
        }
        std::cerr << "\twrite " << dst
                  << " with dtype=" << darr.header().dtype()
                  << " shape: (";
        for (auto dim : darr.header().shape()) {
            std::cerr << " " << dim;
        }
        std::cerr << " ) to " << args.output << std::endl;

        custard::write(outs, dst, fsiz);
        if (!outs) {
            std::cerr << "stream error: " << strerror(errno) << std::endl;
            std::cerr << "failed to write " << dst
                      << "(" << fsiz << ") to "
                      << args.output << "\n" << one << std::endl;
            continue;
        }
        darr.write(outs);
        outs.flush();
    }

    outs.pop();

    return 0;
}
