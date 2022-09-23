#include "WireCellAux/TensorTools.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellAux/SimpleTensorSet.h"

#include "WireCellUtil/String.h"

using namespace WireCell;

template<typename ElementType>
ITensor::pointer make_tensor(const PointCloud::Array& array)
{
    using ST = Aux::SimpleTensor<ElementType>;
    ElementType* data = (ElementType*)array.bytes().data();
    return std::make_shared<ST>(array.shape(), data, array.metadata());
}

ITensor::pointer WireCell::Aux::as_itensor(const PointCloud::Array& array)
{
    if (array.is_type<char>()) return make_tensor<char>(array);
    if (array.is_type<std::byte>()) return make_tensor<std::byte>(array);

    if (array.is_type<int8_t>())  return make_tensor<int8_t>(array);
    if (array.is_type<int16_t>()) return make_tensor<int16_t>(array);
    if (array.is_type<int32_t>()) return make_tensor<int32_t>(array);
    if (array.is_type<int64_t>()) return make_tensor<int64_t>(array);

    if (array.is_type<uint8_t>()) return make_tensor<uint8_t>(array);
    if (array.is_type<uint16_t>()) return make_tensor<uint16_t>(array);
    if (array.is_type<uint32_t>()) return make_tensor<uint32_t>(array);
    if (array.is_type<uint64_t>()) return make_tensor<uint64_t>(array);

    if (array.is_type<float>()) return make_tensor<float>(array);
    if (array.is_type<double>()) return make_tensor<double>(array);
    if (array.is_type<std::complex<float>>()) return make_tensor<std::complex<float>>(array);
    if (array.is_type<std::complex<double>>()) return make_tensor<std::complex<double>>(array);

    return nullptr;
}

ITensorSet::pointer
WireCell::Aux::as_itensorset(const PointCloud::Dataset& dataset)
{
    auto md = dataset.metadata();

    Configuration names = Json::arrayValue;
    
    int ident = get(md, "ident", 0);

    auto sv = std::make_shared<std::vector<ITensor::pointer>>();
    for (const auto& [name, arr] : dataset.store()) {
        sv->push_back(as_itensor(arr));
        names.append(name);
    }

    md["_dataset_arrays"] = names;
    return std::make_shared<Aux::SimpleTensorSet>(ident, md, sv);
}


template<typename ElementType>
PointCloud::Array make_array(const ITensor::pointer& ten, bool share)
{
    ElementType* data = (ElementType*)ten->data();
    PointCloud::Array arr(data, ten->shape(), share);
    arr.metadata() = ten->metadata();
    return arr;
}

PointCloud::Array WireCell::Aux::as_array(const ITensor::pointer& ten, bool share)
{
    const auto& dt = ten->element_type();

    if (dt == typeid(char)) return make_array<char>(ten, share);
    if (dt == typeid(std::byte)) return make_array<std::byte>(ten, share);

    if (dt == typeid(int8_t)) return make_array<int8_t>(ten, share);
    if (dt == typeid(int16_t)) return make_array<int16_t>(ten, share);
    if (dt == typeid(int32_t)) return make_array<int32_t>(ten, share);
    if (dt == typeid(int64_t)) return make_array<int64_t>(ten, share);

    if (dt == typeid(uint8_t)) return make_array<uint8_t>(ten, share);
    if (dt == typeid(uint16_t)) return make_array<uint16_t>(ten, share);
    if (dt == typeid(uint32_t)) return make_array<uint32_t>(ten, share);
    if (dt == typeid(uint64_t)) return make_array<uint64_t>(ten, share);

    if (dt == typeid(float)) return make_array<float>(ten, share);            
    if (dt == typeid(double)) return make_array<double>(ten, share);            
    if (dt == typeid(std::complex<float>)) return make_array<std::complex<float>>(ten, share);            
    if (dt == typeid(std::complex<double>)) return make_array<std::complex<double>>(ten, share);            

    return PointCloud::Array();
}

const std::string dataset_array_list_key = "_dataset_arrays";

PointCloud::Dataset WireCell::Aux::as_dataset(const ITensorSet::pointer& tens, bool share)
{
    PointCloud::Dataset ret;

    auto tmd = tens->metadata();
    Configuration names = tmd[dataset_array_list_key];
    tmd.removeMember(dataset_array_list_key);
    tmd["ident"] = tens->ident();
    ret.metadata() = tmd;

    auto sv = tens->tensors();

    if (names.isNull()) {
        names = Json::objectValue;
        size_t narr = sv->size();
        for (size_t ind=0; ind<narr; ++ind) {
            auto name = sv->at(ind)->metadata()["name"];
            if (name.isNull()) {
                names.append(String::format("array%d", ind));
            }
            else {
                names.append(name);
            }
        }
    }

    size_t narr = names.size();
    for (size_t ind=0; ind < narr; ++ind) {
        std::string name = names[(int)ind].asString();
        ret.add(name, as_array(sv->at(ind), share));
    }
    return ret;    
}

