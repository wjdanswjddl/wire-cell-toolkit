
#include "WireCellPytorch/Util.h"

#include "WireCellUtil/Exceptions.h"
#include "WireCellAux/SimpleTensorSet.h"
#include "WireCellAux/SimpleTensor.h"

using namespace WireCell;

std::string Pytorch::dump(const torch::Tensor &ten)
{
    std::stringstream ss;
    ss << "torch::Tensor: ";
    ss << " shape {"
       << ten.size(0) << ", "
       << ten.size(1) << ", "
       << ten.size(2) << ", "
       << ten.size(3) << "} ";
    return ss.str();
}

ITensorSet::pointer Pytorch::to_itensor(const std::vector<torch::IValue> &inputs)
{
    ITensor::vector *itv = new ITensor::vector;

    int ind = 0;
    for (auto ival : inputs)
    {
        auto ten = ival.toTensor().cpu();
        std::vector<size_t> shape = {(size_t)ten.size(1), (size_t)ten.size(2), (size_t)ten.size(3)};
        // TODO need to figure out type from dtyp
        Aux::SimpleTensor<float> *st = new Aux::SimpleTensor<float>(shape);
        size_t nbyte = 4;
        for (auto n : shape)
            nbyte *= n;
        auto data = (float *)st->data();
        memcpy(data, (float *)ten[0][0].data<float>(), nbyte);
        itv->push_back(ITensor::pointer(st));
        ++ind;
    }

    // FIXME use a correct seqno
    int seqno = 0;
    Configuration md;

    return std::make_shared<Aux::SimpleTensorSet>(seqno, md, ITensor::shared_vector(itv));
}

std::vector<torch::IValue> Pytorch::from_itensor(const ITensorSet::pointer &inputs)
{
    std::vector<torch::IValue> ret;

    for (auto iten : *inputs->tensors())
    {
        if (iten->shape().size() != 4)
        {
            THROW(ValueError() << errmsg{"iten->shape().size()!=4"});
        }
        //TODO determine data type from metadata
        auto ten = torch::from_blob((float *)iten->data(), {(long)iten->shape()[0],
                                                            (long)iten->shape()[1],
                                                            (long)iten->shape()[2],
                                                            (long)iten->shape()[3]});
        ret.push_back(ten);
    }

    return ret;
}