/**
   Translate between C++ types and Numpy dtype strings.

   This restricts to short codes.  i4, f8, etc and is not exhaustive.
 */

#ifndef WIRECELL_UTIL_DTYPE
#define WIRECELL_UTIL_DTYPE

#include <complex>

namespace WireCell {

    // Return a dtype string representing the C++ numeric type NType
    template<typename NType>
    std::string dtype() { return ""; }

    template <> inline std::string dtype<char>() { return "c"; }
    template <> inline std::string dtype<std::byte>() { return "B"; }

    template <> inline std::string dtype<int8_t>()   { return "i1"; }
    template <> inline std::string dtype<int16_t>()  { return "i2"; }
    template <> inline std::string dtype<int32_t>()  { return "i4"; }
    template <> inline std::string dtype<int64_t>()  { return "i8"; }

    template <> inline std::string dtype<uint8_t>()  { return "u1"; }
    template <> inline std::string dtype<uint16_t>() { return "u2"; }
    template <> inline std::string dtype<uint32_t>() { return "u4"; }
    template <> inline std::string dtype<uint64_t>() { return "u8"; }

    template <> inline std::string dtype<float>() { return "f4"; }
    template <> inline std::string dtype<double>() { return "f8"; }
    template <> inline std::string dtype<std::complex<float>>() { return "c8"; }
    template <> inline std::string dtype<std::complex<double>>() { return "c16"; }


}

#endif
