#include "WireCellUtil/Type.h"
#include "WireCellUtil/Testing.h"

#include <iostream>
#include <vector>
using namespace std;
using namespace WireCell;

int main()
{
    int i;
    vector<int> vi;

    Assert(typeid(int) == typeid(i));


    cerr << "int: " << type(i) << endl;
    cerr << "vector<int>: \"" << type(vi) << "\"\n";

    AssertMsg("std::vector<int, std::allocator<int> >" == type(vi), "GCC demangling fails");
}
