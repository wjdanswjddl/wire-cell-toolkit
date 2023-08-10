#include "WireCellAux/BlobTools.h"

using namespace WireCell;

Aux::BlobCategory::BlobCategory(const IBlob::pointer& iblob)
{
    if (!iblob) {
        stat = Status::null;
        return;
    }

    const auto& blob = iblob->shape();
    const auto& strips = blob.strips();
    if (strips.empty()) {
        stat = Status::nostrips;
        return;
    }

    for (const auto& strip : strips) {
        if (strip.bounds.second == strip.bounds.first) {
            stat = Status::nowidth;
            return;
        }
    }
    const auto& corners = blob.corners();
    if (corners.empty()) {
        stat = Status::nocorners;
        return;
    }
    if (corners.size() < 3) {
        stat = Status::novolume;
        return;
    }
    stat = Status::okay;
}
