#include "WireCellUtil/svggpp.hpp"
#include <stdexcept>

#include <regex>

// tag: required SVG element name
const std::string tag_key = "tag";

// attr: optional array of objects providing attributes as key/value, values are string.
const std::string attr_key = "attr";

// body: optional element body.
const std::string body_key = "body";

static
std::vector<std::string> split(const std::string str,
                               const std::string regex_str) {
    std::regex regexz(regex_str);
    return {std::sregex_token_iterator(str.begin(), str.end(), regexz, -1),
            std::sregex_token_iterator()};
}

static
std::string join(const std::vector<std::string>& parts, const std::string& delim=" ")
{
    std::stringstream ss;
    std::string comma="";
    for (const auto& one : parts) {
        ss << comma << one;
        comma = delim;
    }
    return ss.str();
}

using namespace svggpp;

attr_t& svggpp::attr(elem_t& elem)
{
    return elem[attr_key];
}
const attr_t& svggpp::attr(const elem_t& elem)
{
    return elem[attr_key];
}


std::string svggpp::get(const elem_t& elem, const std::string& key, const std::string& def)
{
    const auto& a = attr(elem);
    if (a.contains(key)) {
        return a[key];
    }
    return def;
}

elem_t& svggpp::set(elem_t& elem, const std::string& key, const std::string& value)
{
    attr(elem)[key] = value;
    return elem;
}

std::string svggpp::id(const elem_t& elem)
{
    return get(elem, "id", "");
}

std::string svggpp::link(const elem_t& elem)
{
    return "#" + id(elem);
}


attr_t svggpp::point(double x, double y)
{
    attr_t attrs = {
        {"x", std::to_string(x)},
        {"y", std::to_string(y)}};
    return attrs;
}

attr_t svggpp::point(const point_t& pt) {
    return point(pt.first, pt.second);
}

attr_t svggpp::points(const std::vector<point_t>& pts)
{
    std::stringstream ss;
    std::string space="";
    for (const auto& p : pts) {
        ss << space << p.first << "," << p.second;
        space = " ";
    }
    attr_t attrs = {
        {"points", ss.str()},
    };
    return attrs;
}

attr_t svggpp::center(double cx, double cy)
{
    attr_t attrs = {
        {"cx", std::to_string(cx)},
        {"cy", std::to_string(cy)}};
    return attrs;
}

attr_t svggpp::size(double width, double height)
{
    attr_t attrs = {
        {"width", std::to_string(width)},
        {"height", std::to_string(height)}};
    return attrs;
}


attr_t svggpp::box(double x, double y, double width, double height)
{
    attr_t attrs = point(x,y);
    attrs.update(size(width,height));
    return attrs;
}

attr_t svggpp::endpoints(double x1, double y1, double x2, double y2)
{
    attr_t attrs = {
        {"x1", std::to_string(x1)},
        {"y1", std::to_string(y1)},
        {"x2", std::to_string(x2)},
        {"y2", std::to_string(y2)}};
    return attrs;
}

attr_t svggpp::viewbox(double x1, double x2, double width, double height)
{
    std::stringstream ss;
    ss << x1 << " " << x2 << " " << width << " " << height;
    attr_t attrs = { {"viewBox", ss.str()} };
    return attrs;
}

attr_t svggpp::viewport(const elem_t& svg)
{
    if (! svg.contains(attr_key)) {
        throw std::invalid_argument("no attributes key \""+attr_key+"\" in: " + svg.dump());
    }
    const auto& attr = svg[attr_key];
    if (! attr.contains("viewBox")) {
        throw std::invalid_argument("no \"viewBox\" attribute in: " + svg.dump());
    }
    std::string vb = attr["viewBox"];
    auto parts = split(vb, " ");
    attr_t ret = { {"x", parts[0] },
                   {"y", parts[1] },
                   {"width", parts[2] },
                   {"height", parts[3] } };
    return ret;
}


attr_t svggpp::css_style(const std::string& css)
{
    attr_t attrs = { {"style",css} };
    return attrs;
}

attr_t svggpp::css_class(const std::string& cname)
{
    attr_t attrs = { {"class",cname} };
    return attrs;
}

attr_t svggpp::css_class(const std::vector<std::string>& cnames)
{
    return css_class(join(cnames));
}



//
// Eleme
// 

static void assert_is_attr(const svggpp::attr_t& one)
{
    for (auto& [key, val] : one.items()) {
        if (! val.is_string()) {
            throw std::invalid_argument("illegal svg attribute value type");
        }
    }
}
static void assert_is_elem(const svggpp::elem_t& one)
{
    if (one.empty()) {
        throw std::invalid_argument("element is null");
    }
    if (one.is_array()) {
        throw std::invalid_argument("element is array");
    }
    if (one.is_object()) {
        if (! one.contains(tag_key)) {
            throw std::invalid_argument("element has no tag");
        }
        if (! one[tag_key].is_string()) {
            throw std::invalid_argument("element tag is not a string");
        }
        if (one.contains(attr_key)) {
            if (! one[attr_key].is_object()) {
                throw std::invalid_argument("element attributes not an object");            
            }
        }
        if (one.contains(body_key)) {
            if (! one[body_key].is_array()) {
                throw std::invalid_argument("element body not an array");            
            }
        }
        return;
    }
    // all other types okay (string, number, bool)
}

svggpp::elem_t svggpp::element(const std::string& tag,
                               const svggpp::attr_t& cattrs,
                               const svggpp::elem_t& cbody)
{
    attr_t attrs = cattrs;
    elem_t body = cbody;
    elem_t elem = {{tag_key,tag},
                   {attr_key, attr_t::object()},
                   {body_key, elem_t::array()}};

    // attrs
    if (attrs.empty()) { attrs = attr_t::object(); }
    if (attrs.is_object()) {
        attrs = attr_t::array({attrs});
    }
    if (attrs.is_array()) {
        for (auto& one : attrs) {
            assert_is_attr(one);
            elem[attr_key].update(one);
        }
    }

    // body
    if (body.empty()) { body = elem_t::array(); }
    if (! body.is_array()) {
        body = elem_t::array({body});
    }
    for (auto& one : body) {
        if (one.empty()) {
            continue;
        }
        if (one.is_object()) {
            assert_is_elem(one);
        }
        elem[body_key].push_back(one);
    }

    return elem;
}

elem_t& svggpp::update(elem_t& elem, attr_t attrs)
{
    elem[attr_key].update(attrs);
    return elem;
}

elem_t& svggpp::append(elem_t& parent, const elem_t& cchild)
{
    elem_t child = cchild;
    if (! child.is_array()) {
        child = elem_t::array({child});
    }
    for (auto& c : child) {
        parent[body_key].push_back(c);
    }
    return parent;
}

elem_t svggpp::text(const std::string& t, double x, double y, attr_t attrs)
{
    attrs.update(svggpp::point(x,y));
    return element("text", attrs, t);
}

elem_t svggpp::circle(double cx, double cy, double r, attr_t attrs)
{
    attrs.update(svggpp::center(cx,cy));
    attrs.update({ {"r", std::to_string(r) } });
    return element("circle", attrs);
}
elem_t svggpp::circle(point_t cen, double r, attr_t attrs)
{
    return circle(cen.first, cen.second, r, attrs);
}
elem_t svggpp::line(double x1, double y1, double x2, double y2,
                    attr_t attrs)
{
    attrs.update(svggpp::endpoints(x1,y1,x2,y2));
    return element("line", attrs);
}

elem_t svggpp::polygon(const std::vector<point_t>& pts, attr_t attrs)
{
    attrs.update(svggpp::points(pts));
    return element("polygon", attrs);
}

elem_t svggpp::style(const std::string& css)
{
    return element("style", nullptr, css);
}

elem_t svggpp::view(const std::string& id, attr_t attrs, elem_t body)
{
    attrs["id"] = id;
    return element("view", attrs, body);
}        

elem_t svggpp::anchor(const std::string& url, attr_t attrs, elem_t body)
{
    attrs["href"] = url;
    return element("a", attrs, body);
}

elem_t svggpp::group(attr_t attrs, elem_t body)
{
    return element("group", attrs, body);
}

elem_t svggpp::svg(attr_t attrs, elem_t body)
{
    return element("svg", attrs, body);
}

elem_t svggpp::svg(double x, double y, double width, double height, 
                   attr_t attrs, elem_t body)
{
    attrs.update(svggpp::point(x,y));
    attrs.update(svggpp::size(width,height));
    return element("svg", attrs, body);
}
elem_t svggpp::use(const std::string& url)
{
    return element("use", { {"href",url} });
}

elem_t svggpp::defs(elem_t body)
{
    return element("defs", {}, body);
}

//
// I/O
//

std::string svggpp::dumps_level(const elem_t& elem, int indent, int level)
{
    if (elem.is_null()) return "";

    std::stringstream ss;
    const std::string tab(indent*level, ' ');

    if (elem.is_array()) {
        for (const auto& sub : elem) {
            ss << dumps_level(sub, indent, level); // don't indent one more 
                                                          }
        return ss.str();
    }

    if (elem.is_object()) {
        if (! elem.contains(tag_key)) {
            throw std::invalid_argument("no key \"" + tag_key + "\" in " + elem.dump() );
        }
        std::string tag = elem[tag_key];
        ss << tab << "<" << tag;
        if (elem.contains(attr_key)) {
            for (const auto& kv : elem[attr_key].items()) {
                ss << " " << kv.key() << "=" << kv.value();
            }
        }
        if (elem.contains(body_key)) {
            ss << ">\n";
            ss << dumps_level(elem[body_key], indent, level+1);
            ss << tab << "</" << tag << ">\n";
            return ss.str();
        }
        ss << "/>\n";
        return ss.str();
    }

    if (elem.is_string()) {
        std::string s = elem;
        ss << s << "\n"; // flush left
        return ss.str();
    }

    // number, boolean
    ss << tab << elem.dump() << "\n";
    return ss.str();
}

std::string svggpp::dumps(const elem_t& elem, int indent)
{
    return dumps_level(elem, indent);
}
