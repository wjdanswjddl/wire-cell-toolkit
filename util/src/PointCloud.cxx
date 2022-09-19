#include "WireCellUtil/PointCloud.h"

using namespace WireCell::PointCloud;

bool Dataset::add(const std::string& name, const Array& arr)
{
    size_t n = num_elements();
    if (!n) {
        m_store.insert({name,arr});
        return true;
    }
    if (n != arr.num_elements()) {
        THROW(ValueError() << errmsg{"array size mismatch when adding to dataset"});
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
        THROW(ValueError() << errmsg{"array size mismatch when adding to dataset"});
    }
    auto it = m_store.find(name);
    if (it == m_store.end()) {
        m_store.insert({name,arr});
        return true;
    }
    it->second = arr;
    return false;
}
Dataset::selection_t Dataset::selection(const std::vector<std::string>& names) const
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
    size_t pad = 0;

    // Remember any missing keys or empty tail arrays.
    std::vector<std::string> topad;

    for (auto& [key, arr] : m_store) {
        auto t = tail.find(key);
        if (t == tail.end()) {
            topad.push_back(key);
            continue;
        }
        auto bytes = t->second.bytes();
        const size_t nbytes = bytes.size();
        if (nbytes == 0) {
            topad.push_back(key);
            continue;
        }
        if (pad == 0) { // first non-zero
            pad = nbytes;
        }
        if (nbytes >= pad) { // equal or truncate
            arr.append(bytes.data(), pad);
            continue;
        }
        // The array is nonzero but short.
        arr.append(bytes.data(), nbytes);
        std::vector<std::byte> zeros(pad - nbytes, std::byte{0});
        arr.append(zeros.begin(), zeros.end());
    }
            
    if (pad == 0) {
        return;         // no matching, non-empty in tail
    }

    std::vector<std::byte> zeros(pad, std::byte{0});
    for (const auto& tkey : topad) {
        m_store[tkey].append(zeros);
    }
}
