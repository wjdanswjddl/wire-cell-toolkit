/*
  A simple API for generating SVG.

  It makes use of JSON for Modern C++ (nlohmann/json) as an
  intermediate representation which may be converted to persistent SVG
  XML.

*/

#ifndef SVGPPP_HPP
#define SVGPPP_HPP

// Fixme: we ride on custard's coat tails to provide json.hpp,
// eventually this will become a first class citizen in WCT
#include "WireCellUtil/custard/nlohmann/json.hpp"

namespace svggpp {

    // An element is a json object.
    using elem_t = nlohmann::json;
    // with these object attributes
    // 
    // tag: required SVG element name
    const std::string tag_key = "tag";

    // attr: optional array of objects providing attributes as key/value, values are string.
    const std::string attr_key = "attr";

    // body: optional element body.
    const std::string body_key = "body";

    // A disembodied element attribute set
    using attrs_t = nlohmann::json;

    // Provide attribute fragments for top, file-level <svg>
    const attrs_t svg_header = {{"version","1.1"}, {"xmlns","http://www.w3.org/2000/svg"}};


    // Functions to query elements, returning default value if fail.

    std::string attr(const elem_t& elem, const std::string& key, const std::string& def="");
    std::string id(const elem_t& elem);
    std::string link(const elem_t& elem);

    // Function to return attribute object fragments to foist SVG
    // schema into C++.

    using point_t = std::pair<double,double>;

    namespace Attr {

        attrs_t point(double x, double y);
        attrs_t point(const point_t& pt);
        attrs_t points(const std::vector<point_t>& pts);
        attrs_t center(double cx, double cy);
        attrs_t size(double width, double height);
        attrs_t box(double x, double y, double width, double height);
        attrs_t endpoints(double x1, double y1, double x2, double y2);
        attrs_t viewbox(double x1, double x2, double width, double height);
        attrs_t style(const std::string& css);
        attrs_t klass(const std::string& cname);
        attrs_t klass(const std::vector<std::string>& cnames);

    } // Attr

    namespace Elem {

        // Return <tag> element with attributes attrs and body.
        svggpp::elem_t element(const std::string& tag,
                               const svggpp::attrs_t& attrs = nullptr,
                               const svggpp::elem_t& body = nullptr);

        // Append child to body of parent, return parent.  Parent body
        // will be an array type with any initial non-array contents
        // as first element.
        elem_t append(elem_t& parent, const elem_t& child);

        // Following functions foist specific SVG tags to C++ function API.
        elem_t text(const std::string& t, double x=0, double y=0, attrs_t attrs = {});
        elem_t circle(double cx=0, double cy=0, double r=1, attrs_t attrs = {});
        elem_t line(double x1=0, double y1=0, double x2=1, double y2=1,
                    attrs_t attrs = { {"stroke","black"} });
        elem_t polygon(const std::vector<point_t>& pts, attrs_t attrs = {});
        // for now, treat css as opaque string
        elem_t style(const std::string& css);
        elem_t view(const std::string& id, attrs_t attrs = {}, elem_t body = {});
        elem_t anchor(const std::string& url, attrs_t attrs = {}, elem_t body = {});
        elem_t svg(attrs_t attrs = svg_header, elem_t body=nullptr);
        elem_t svg(double x, double y, double width, double height, 
                   attrs_t attrs = svg_header, elem_t body=nullptr);
    } // Elem

    

    std::string dumps_level(const elem_t& elem, int indent=4, int level=0);
    // Return SVG XML string representation of the element.

    std::string dumps(const elem_t& elem, int indent=4);

}

#endif
