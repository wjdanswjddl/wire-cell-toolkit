/*
  A simple API for generating SVG.

  It makes use of JSON for Modern C++ (nlohmann/json) as an
  intermediate representation of XML.

*/

#ifndef SVGPPP_HPP
#define SVGPPP_HPP

// Fixme: we ride on custard's coat tails to provide json.hpp,
// eventually this will become a first class citizen in WCT
#include "WireCellUtil/custard/nlohmann/json.hpp"

namespace svggpp {

    // Represent SVG XML elements.  Depending on context, it may be of
    // (json) type string, object or array.  If object type it must
    // have keys: "tag" of type string, "attr" of type attr_t and
    // "body" of type elem_t.  If array, elements must be of type
    // string or object.
    using elem_t = nlohmann::json;

    // Represent a SVG XML attributes.  It must be of type object with
    // both key and value types of string.
    using attr_t = nlohmann::json;

    // Provide attribute fragments for top, file-level <svg>
    const attr_t svg_header = {{"version","1.1"}, {"xmlns","http://www.w3.org/2000/svg"}};

    // Return attribute set of element.
    attr_t& attr(elem_t& elem);
    const attr_t& attr(const elem_t& elem);

    // Return element attribute value with key or def if key not in attributes.
    std::string get(const elem_t& elem, const std::string& key, const std::string& def);

    // SVG values are always strings, even when they are numbers.
    // This wraps up the double cast to get them as a number type.
    template<typename T>
    T cast(const attr_t& val)
    {
        return attr_t::parse(val.get<std::string>()).get<T>();
    }

    // Set element's attribute key to value.
    elem_t& set(elem_t& elem, const std::string& key, const std::string& value);


    // Return the ID or empty string
    std::string id(const elem_t& elem);
    std::string link(const elem_t& elem);

    // represent a point in 2-space
    using point_t = std::pair<double,double>;

    // Function to return attribute object fragments to foist SVG
    // schema into C++.

    // Attribute generators return an attributes object
    attr_t point(double x, double y);
    attr_t point(const point_t& pt);
    attr_t points(const std::vector<point_t>& pts);
    attr_t center(double cx, double cy);
    attr_t size(double width, double height);
    attr_t box(double x, double y, double width, double height);
    attr_t endpoints(double x1, double y1, double x2, double y2);
    // Construct a "viewBox" attribute as "x y width height"
    // string list.
    attr_t viewbox(double x, double y, double width, double height);
    // Extract "viewBox" of svg as a "view port" object with
    // keys "x", "y", "width" and "height" and values as strings.
    attr_t viewport(const elem_t& elem);
    // Reformat a view port object to {"viewBox":"x y w h"} form.
    attr_t viewbox(const attr_t& vp);

    /// Produce a CSS style attribute.  Note, there is also a <style>
    /// element.  For now, CSS is treated only as an opaque string.
    attr_t css_style(const std::string& css);
    /// Return a CSS "class" attribute
    attr_t css_class(const std::string& cname);
    attr_t css_class(const std::vector<std::string>& cnames);


    /// Create and return a new element.  User code should only call
    /// this function to construct new elements.  The attrs and body
    /// may evalute to false.  The attrs may be an object or array of
    /// object.  The value of the attributes of these objects must be
    /// of type string.  The body may be of type elem_t object or
    /// string or number or array of these.
    svggpp::elem_t element(const std::string& tag,
                           const svggpp::attr_t& attrs = attr_t::object(),
                           const svggpp::elem_t& body = elem_t::array());

    /// Append child to parent body, return parent.  Parent body will
    /// be an array of elements.  Child may be scalar or array.  See
    /// comments on elem_t above.
    elem_t& append(elem_t& parent, const elem_t& child);

    /// Update attributes of element, returning elem.
    elem_t& update(elem_t& elem, attr_t attrs);

    /// Following functions foist specific SVG tags to a C++ function
    /// API.

    /// Shape things:
        
    elem_t text(const std::string& t, double x=0, double y=0, attr_t attrs = {});
    elem_t circle(double cx, double cy, double r=1, attr_t attrs = {});
    elem_t circle(point_t cen, double r=1, attr_t attrs = {});
    elem_t line(double x1=0, double y1=0, double x2=1, double y2=1,
                attr_t attrs = { {"stroke","black"} });
    elem_t polygon(const std::vector<point_t>& pts, attr_t attrs = {});

    /// Abstract things and grouping

    /// Produces a <style> element.  Note there is also a style
    /// attribute, see css_style().  For now, CSS is treated only as
    /// an opaque string.
    elem_t style(const std::string& css);
    elem_t view(const std::string& id, attr_t attrs = {}, elem_t body = {});
    elem_t anchor(const std::string& url, attr_t attrs = {}, elem_t body = {});
    /// A group <g> element.
    elem_t group(attr_t attrs = {}, elem_t body = {});
    /// An <svg> element
    elem_t svg(attr_t attrs = svg_header, elem_t body=nullptr);
    elem_t svg(double x, double y, double width, double height, 
               attr_t attrs = svg_header, elem_t body=nullptr);
    /// Use some element by referencing it's URL, typically an
    /// "#name" ID.  See Attr::link()
    elem_t use(const std::string& url);
    /// A defs is a spot to park elements for later reference via <use>
    elem_t defs(elem_t body = {});


    
    // I/O
    
    std::string dumps_level(const elem_t& elem, int indent=4, int level=0);

    std::string dumps(const elem_t& elem, int indent=4);

}



#endif
