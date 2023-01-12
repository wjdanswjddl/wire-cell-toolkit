#include "WireCellUtil/svggpp.hpp"

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

std::string svggpp::attr(const elem_t& elem, const std::string& key, const std::string& def)
{
    if (elem.contains(attr_key)) {
        const auto& a = elem[attr_key];
        if (a.contains(key)) {
            return a[key];
        }
    }
    return def;
}

std::string svggpp::id(const elem_t& elem)
{
    return attr(elem, "id");
}

std::string svggpp::link(const elem_t& elem)
{
    return "#" + id(elem);
}


attrs_t Attr::point(double x, double y)
{
    attrs_t attrs = {
        {"x", std::to_string(x)},
        {"y", std::to_string(y)}};
    return attrs;
}

attrs_t Attr::point(const point_t& pt) {
    return point(pt.first, pt.second);
}

attrs_t Attr::points(const std::vector<point_t>& pts)
{
    std::stringstream ss;
    std::string space="";
    for (const auto& p : pts) {
        ss << space << p.first << "," << p.second;
        space = " ";
    }
    attrs_t attrs = {
        {"points", ss.str()},
    };
    return attrs;
}

attrs_t Attr::center(double cx, double cy)
{
    attrs_t attrs = {
        {"cx", std::to_string(cx)},
        {"cy", std::to_string(cy)}};
    return attrs;
}

attrs_t Attr::size(double width, double height)
{
    attrs_t attrs = {
        {"width", std::to_string(width)},
        {"height", std::to_string(height)}};
    return attrs;
}

attrs_t Attr::box(double x, double y, double width, double height)
{
    attrs_t attrs = point(x,y);
    attrs.update(size(width,height));
    return attrs;
}

attrs_t Attr::endpoints(double x1, double y1, double x2, double y2)
{
    attrs_t attrs = {
        {"x1", std::to_string(x1)},
        {"y1", std::to_string(y1)},
        {"x2", std::to_string(x2)},
        {"y2", std::to_string(y2)}};
    return attrs;
}

attrs_t Attr::viewbox(double x1, double x2, double width, double height)
{
    std::stringstream ss;
    ss << x1 << " " << x2 << " " << width << " " << height;
    attrs_t attrs = { {"viewBox", ss.str()} };
    return attrs;
}

attrs_t Attr::style(const std::string& css)
{
    return { {"style",css} };
}

attrs_t Attr::klass(const std::string& cname)
{
    return { {"class",cname} };
}

attrs_t Attr::klass(const std::vector<std::string>& cnames)
{
    return klass(join(cnames));
}


svggpp::elem_t Elem::element(const std::string& tag,
                             const svggpp::attrs_t& attrs,
                             const svggpp::elem_t& body)
{
    elem_t elem = {{tag_key,tag}};

    if (attrs.is_object()) {
        elem[attr_key] = attrs;
    }

    if (body.is_null()) {
        return elem;
    }
        
    if (body.is_object()) {
        elem[body_key] = elem_t::array({body});
        return elem;
    }

    elem[body_key] = body;
    return elem;
}

elem_t Elem::append(elem_t& parent, const elem_t& child)
{
    elem_t children = elem_t::array({});

    if (parent.contains(body_key)) {
        auto body = parent[body_key];
        if (body.is_array()) {
            children = body;
        }
        else if (! body.is_null()) {
            children.push_back(body);
        }
    }
    if (child.is_array()) {
        children.update(child.begin(), child.end());
    }
    else if (! child.is_null()) {
        children.push_back(child);
    }
    parent[body_key] = children;
    return parent;
}


elem_t Elem::text(const std::string& t, double x, double y, attrs_t attrs)
{
    attrs.update(Attr::point(x,y));
    return element("text", attrs, t);
}

elem_t Elem::circle(double cx, double cy, double r, attrs_t attrs)
{
    attrs.update(Attr::center(cx,cy));
    attrs.update({ {"r", std::to_string(r) } });
    return element("circle", attrs);
}

elem_t Elem::line(double x1, double y1, double x2, double y2,
                  attrs_t attrs)
{
    attrs.update(Attr::endpoints(x1,y1,x2,y2));
    return element("line", attrs);
}

elem_t Elem::polygon(const std::vector<point_t>& pts, attrs_t attrs)
{
    attrs.update(Attr::points(pts));
    return element("polygon", attrs);
}

elem_t Elem::style(const std::string& css)
{
    return element("style", nullptr, css);
}

elem_t Elem::view(const std::string& id, attrs_t attrs, elem_t body)
{
    attrs["id"] = id;
    return element("view", attrs, body);
}        

elem_t Elem::anchor(const std::string& url, attrs_t attrs, elem_t body)
{
    attrs["href"] = url;
    return element("a", attrs, body);
}

elem_t Elem::svg(attrs_t attrs, elem_t body)
{
    return element("svg", attrs, body);
}

elem_t Elem::svg(double x, double y, double width, double height, 
                 attrs_t attrs, elem_t body)
{
    attrs.update(Attr::point(x,y));
    attrs.update(Attr::size(width,height));
    return element("svg", attrs, body);
}

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
