#include "WireCellAux/TensorFrame.h"
#include "WireCellAux/TensorTools.h"

#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(TensorFrame, WireCell::Aux::TensorFrame,
                 WireCell::ITensorSetFrame)

using namespace WireCell;
using namespace WireCell::Aux;

TensorFrame::TensorFrame()
{
}
TensorFrame::~TensorFrame()
{
}


bool TensorFrame::operator()(const input_pointer& tens, output_pointer& frame)
{
    frame = nullptr;
    if (! tens) {
        // eos
        return true;
    }
    frame = to_iframe(tens);
    return true;
}
