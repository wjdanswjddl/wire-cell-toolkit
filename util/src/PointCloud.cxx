#include "WireCellUtil/PointCloud.h"

using namespace WireCell::PointCloud;

//// array

static bool append_compatible(const Array::shape_t& s1, const Array::shape_t& s2)
{
    size_t size = s1.size();

    if (size != s2.size()) return false; // same dimensions
    if (size == 1) return true;          // all 1D append okay
    for (size_t ind=1; ind<size; ++ind) {
        if (s1[ind] != s2[ind]) return false;
    }
    return true;
}

void Array::append(const Array& tail)
{
    const auto tshape = tail.shape();
    if (not append_compatible(m_shape, tshape)) {
        THROW(ValueError() << errmsg{"array append with incompatible shape"});
    }
    append(tail.bytes());
}

void Array::append(const std::byte* data, size_t nbytes)
{
    if (nbytes % m_ele_size) {
        THROW(ValueError() << errmsg{"byte append not compatible with existing type"});
    }
    const size_t nelem = nbytes / m_ele_size;

    size_t rows = 0, notrows=1;
    for (auto s : m_shape) {
        if (!rows) {
            rows = s;
            continue;
        }
        notrows *= s;
    }

    if (nelem % notrows) {
        THROW(ValueError() << errmsg{"byte append not compatible with existing shape"});
    }
    const size_t nrows = nelem/notrows;

    assure_mutable();
    m_store.insert(m_store.end(), data, data+nbytes);
    m_shape[0] += nrows;
    update_span();
}



//// dataset

bool Dataset::add(const std::string& name, const Array& arr)
{
    size_t nele = num_elements();
    if (!nele) {
        m_store.insert({name,arr});
        return true;
    }
    if (nele != arr.shape()[0]) {
        THROW(WireCell::ValueError() << WireCell::errmsg{"row size mismatch when adding \""+name+"\" to dataset"});
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
    size_t nele = num_elements();
    if (!nele) {
        m_store.insert({name,arr});
        return true;
    }
    if (nele != arr.shape()[0]) {
        THROW(WireCell::ValueError() << WireCell::errmsg{"row size mismatch when moving \""+name+"\" to dataset"});
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


Dataset::keys_t store_keys(const Dataset::store_t& store)
{
    Dataset::keys_t ret;
    for (const auto& [k,v] : store) {
        ret.insert(k);
    }
    return ret;
}

Dataset::keys_t difference(const Dataset::keys_t& a, const Dataset::keys_t& b)
{
    Dataset::keys_t diff;
    std::set_difference(a.begin(), a.end(),
                        b.begin(), b.end(),
                        std::inserter(diff, diff.end()));
    return diff;
}

void Dataset::append(const std::map<std::string, Array>& tail)
{
    if (tail.empty()) return;

    const size_t nele_before = num_elements();

    auto diff = difference(keys(), store_keys(tail));
    if (diff.size() > 0) {
        THROW(ValueError() << errmsg{"missing keys in append"});
    }

    size_t nele = 0;            // check rectangular
    for (auto& [key, arr] : tail) {
        if (nele == 0) {        // first
            nele = arr.shape()[0];
            continue;
        }
        if (arr.shape()[0] != nele) {
            THROW(ValueError() << errmsg{"non-rectangular array in append"});
        }
    }

    for (auto& [key, arr] : m_store) {
        const auto& [ok, oa] = *tail.find(key);
        arr.append(oa.bytes());
    }

    const size_t nele_after = num_elements();

    for (auto cb : m_append_callbacks) {
        cb(nele_before, nele_after);
    }
}

