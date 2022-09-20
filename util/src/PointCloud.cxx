#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/Exceptions.h"

using namespace WireCell::PointCloud;

bool Dataset::add(const std::string& name, const Array& arr)
{
    size_t n = num_elements();
    if (!n) {
        m_store.insert({name,arr});
        return true;
    }
    if (n != arr.num_elements()) {
        THROW(WireCell::ValueError() << WireCell::errmsg{"array size mismatch when adding to dataset"});
    }
    auto it = m_store.find(name);
    if (it == m_store.end()) {
        m_store.insert({name,arr});
        return true;
    }
    it->second = arr;
    return false;
}

bool Dataset::add(const std::string& name, Array&& arr)
{
    size_t n = num_elements();
    if (!n) {
        m_store.insert({name,arr});
        return true;
    }
    if (n != arr.num_elements()) {
        THROW(WireCell::ValueError() << WireCell::errmsg{"array size mismatch when adding to dataset"});
    }
    auto it = m_store.find(name);
    if (it == m_store.end()) {
        m_store.insert({name,arr});
        return true;
    }
    it->second = arr;
    return false;
}
selection_t Dataset::selection(const std::vector<std::string>& names) const
{
    selection_t ret;
    for (const auto& name : names) {
        auto it = m_store.find(name);
        if (it == m_store.end()) {
            return selection_t();
        }
        ret.push_back(std::cref(it->second));
    }
    return ret;
}
Dataset::keys_t Dataset::missing(const Dataset& other)
{
    auto mine = keys();
    auto your = other.keys();
    keys_t diff;
    std::set_difference(mine.begin(), mine.end(),
                        your.begin(), your.end(),
                        std::inserter(diff, diff.end()));
    return diff;
}


void Dataset::append(const std::map<std::string, Array>& tail)
{
    size_t nele_pad=0, nbytes_pad = 0; // tail lengths
    const size_t nprior = num_elements();

    // Remember any missing keys or empty tail arrays.
    std::vector<std::string> topad;

    for (auto& [key, arr] : m_store) {
        auto t = tail.find(key);
        if (t == tail.end()) {
            // no user array, will fully pad
            topad.push_back(key);
            continue;
        }
        auto bytes = t->second.bytes();
        const size_t nbytes = bytes.size();
        if (nbytes == 0) {
            // empty user array, will fully pad
            topad.push_back(key);
            continue;
        }
        // exists and non-zero
        if (nbytes_pad == 0) { // first non-zero
            nbytes_pad = nbytes;
            nele_pad = t->second.num_elements();
        }
        if (nbytes >= nbytes_pad) { // equal or truncate
            arr.append(bytes.data(), nbytes_pad);
            continue;
        }
        // The array is nonzero but short.
        arr.append(bytes.data(), nbytes);
        std::vector<std::byte> zeros(nbytes_pad - nbytes, std::byte{0});
        arr.append(zeros.begin(), zeros.end());
    }
            
    if (nbytes_pad == 0) {
        return;         // no matching in tail or tail length zero
    }

    std::vector<std::byte> zeros(nbytes_pad, std::byte{0});
    for (const auto& tkey : topad) {
        m_store[tkey].append(zeros);
    }

    for (auto cb : m_append_callbacks) {
        cb(nprior, nprior+nele_pad);
    }
}

