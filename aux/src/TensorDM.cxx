#include "WireCellAux/TensorDM.h"
#include "WireCellAux/SimpleTensorSet.h"
#include "WireCellAux/SimpleTensor.h"
#include <regex>

using namespace WireCell;

ITensor::pointer WireCell::Aux::TensorDM::make_metadata_tensor(
    const std::string& datatype,
    const std::string& datapath,
    Configuration metadata)
{
    metadata["datatype"] = datatype;
    metadata["datapath"] = datapath;
    return std::make_shared<Aux::SimpleTensor>(metadata);
}


ITensor::pointer
WireCell::Aux::TensorDM::top_tensor(const ITensor::vector& tens,
                                    const std::string& datatype,
                                    const std::string& datapath)
{
    for (const auto& iten : tens) {
        const auto& tenmd = iten->metadata();
        const auto dtype = tenmd["datatype"].asString();
        const auto dpath = tenmd["datapath"].asString();
        if (dtype == datatype and (datapath.empty() or datapath == dpath)) {
            return iten;
        }
    }
    raise<ValueError>("no array of datatype \"%s\" at datapath \"%s\"",
                      datatype, datapath);
    return nullptr;
}

//     /** Return a map from datapath to tensor.
//       *
//       *   If datatypes is provided, only map types in set.
//       */
//     std::unordered_map<std::string, ITensor::pointer> index(
//         const ITensor::vector& tens,
//         const std::unordered_set<std::string>& datatypes = {});
// std::unordered_map<std::string, ITensor::pointer> WireCell::Aux::TensorDM::index(
//     const ITensor::vector& tens,
//     const std::unordered_set<std::string>& datatypes)
// {
//     std::unordered_map<std::string, ITensor::pointer> ret;
//     for (const auto& iten : tens) {
//         const auto& md = iten->metadata();
//         if (datatypes.empty() or datatypes.find(md["datatype"]) != datatypes.end()) {
//             ret[md["datapath"]] = iten;
//         }            
//     }
//     return ret;
// }


ITensorSet::pointer
WireCell::Aux::TensorDM::as_tensorset(const ITensor::vector& tens,
                                      int ident,
                                      const Configuration& tsetmd)
{
    auto sv = std::make_shared<ITensor::vector>(tens.begin(), tens.end());
    return std::make_shared<SimpleTensorSet>(ident, tsetmd, sv);
}


WireCell::Aux::TensorDM::located_t
WireCell::Aux::TensorDM::index_datapaths(const ITensor::vector& tens)
{
    WireCell::Aux::TensorDM::located_t located;
    for (const auto& iten : tens) {
        auto md = iten->metadata();
        auto jdp = md["datapath"];
        if (!jdp.isString()) {
            continue;
        }
        auto dp = jdp.asString();
        located[dp] = iten;
    }
    return located;
}

ITensor::pointer
WireCell::Aux::TensorDM::first_of(const ITensor::vector& tens,
                                  const std::string& datatype)
{
    for (const auto& iten : tens) {
        auto md = iten->metadata();
        auto jdt = md["datatype"];
        if (!jdt.isString()) {
            continue;
        }
        auto dt = jdt.asString();
        if (dt == datatype) {
            return iten;
        }
    }
    return nullptr;
}
ITensor::vector
WireCell::Aux::TensorDM::match_at(const ITensor::vector& tens,
                                  const std::string& datapath)
{
    std::regex re(datapath);
    ITensor::vector ret;
    for (const auto& iten : tens) {
        auto md = iten->metadata();
        auto jdp = md["datapath"];
        if (!jdp.isString()) {
            continue;
        }
        auto dp = jdp.asString();
        if (! std::regex_match(dp, re)) continue;
        ret.push_back(iten);
    }
    return ret;
}

