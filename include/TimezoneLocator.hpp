#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace tzloc {

namespace bg  = boost::geometry;
namespace bgi = boost::geometry::index;

// Note:
//   - We treat coordinates as planar lon/lat (x = lon, y = lat). For timezone boundaries
//     this is common and works well in practice. If you need exact geodesic semantics,
//     you could switch to geographic CS + strategies later.
using Point   = bg::model::d2::point_xy<double>;
using Polygon = bg::model::polygon<Point, /*Clockwise?*/ false, /*Closed?*/ true>;
using Box     = bg::model::box<Point>;

// R-tree stores (Box, polygon_id)
using RTree = bgi::rtree<std::pair<Box, int>, bgi::quadratic<16>>;

/**
 * @brief Singleton timezone locator with baked-in timezone boundaries.
 *
 * Build-time tooling generates a translation unit that fills polygon + tzname arrays.
 * At runtime, we construct an R-tree using the packed constructor for efficient queries.
 */
class TimezoneLocator {
public:
    // Thread-safe Meyers singleton. Construction happens once on first use.
    static const TimezoneLocator& instance();

    /**
     * @brief Query all timezones covering a point (lat, lon).
     * @param latitude  in degrees  (-90..90)
     * @param longitude in degrees (-180..180] (wrap is tolerated but not required)
     * @param include_boundary If true, points on the boundary count as inside (covered_by).
     *                         If false, requires strict interior (within).
     * @return vector of timezone names (tzids). Multiple names if overlapping polygons.
     */
    std::vector<std::string> query(double latitude,
                                   double longitude,
                                   bool include_boundary = true) const;

    // Convenience: (lon, lat) overload if you prefer GeoJSON ordering.
    std::vector<std::string> queryLonLat(double longitude,
                                         double latitude,
                                         bool include_boundary = true) const {
        return query(latitude, longitude, include_boundary);
    }

private:
    TimezoneLocator(); // defined in .cpp; builds the index from generated data

    static Box bboxOf(const Polygon& poly);

    std::vector<Polygon> polys_;
    std::vector<std::string> tzids_;  // tzids_[i] corresponds to polys_[i]
    RTree rtree_;
};

} // namespace tzloc
