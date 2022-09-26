#include "WireCellUtil/Dtype.h"

size_t WireCell::dtype_size(const std::string& dt)
{
    if (dt == "c") return sizeof(char);
    if (dt == "B") return sizeof(std::byte);
    if (dt == "i1" or dt == "u1") return sizeof(int8_t);
    if (dt == "i2" or dt == "u2") return sizeof(int16_t);
    if (dt == "i4" or dt == "u4") return sizeof(int32_t);
    if (dt == "i8" or dt == "u8") return sizeof(int64_t);
    if (dt == "f4") return sizeof(float);
    if (dt == "f8") return sizeof(double);
    if (dt == "c8") return sizeof(std::complex<float>);
    if (dt == "c16") return sizeof(std::complex<double>);
    return 0;
}
