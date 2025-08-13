#pragma once
#include <string>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace tzloc {

namespace bg  = boost::geometry;
namespace bgi = boost::geometry::index;

using Point   = bg::model::d2::point_xy<double>;
using Polygon = bg::model::polygon<Point, /*Clockwise?*/ false, /*Closed?*/ true>;
using Box     = bg::model::box<Point>;
using RTree   = bgi::rtree<std::pair<Box, int>, bgi::quadratic<16>>;

class TimezoneLocator {
public:
    static const TimezoneLocator& instance();

    std::vector<std::string> query(double latitude,
                                   double longitude,
                                   bool include_boundary = true) const;

    std::vector<std::string> queryLonLat(double longitude,
                                         double latitude,
                                         bool include_boundary = true) const {
        return query(latitude, longitude, include_boundary);
    }

private:
    TimezoneLocator();  // builds from embedded blob (no file I/O)
    static Box bboxOf(const Polygon& poly);

    std::vector<Polygon> polys_;
    std::vector<std::string> tzids_;
    RTree rtree_;
};

} // namespace tzloc
