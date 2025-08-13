#pragma once
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

namespace boost { namespace serialization {

// point_xy<double>
template<class Archive>
void serialize(Archive& ar, boost::geometry::model::d2::point_xy<double>& p, unsigned) {
    double x = p.x(), y = p.y();
    ar & x;
    ar & y;
    if constexpr(Archive::is_loading::value) {
        p.x(x); p.y(y);
    }
}

// polygon<Point,false,true>
template<class Archive, typename Point>
void save(Archive& ar, const boost::geometry::model::polygon<Point, false, true>& poly, unsigned) {
    // exterior
    const auto& outer = poly.outer();
    std::size_t n_outer = outer.size();
    ar & n_outer;
    for (const auto& pt : outer) ar & const_cast<Point&>(pt);

    // holes
    const auto& inners = poly.inners();
    std::size_t n_inners = inners.size();
    ar & n_inners;
    for (const auto& hole : inners) {
        std::size_t n = hole.size();
        ar & n;
        for (const auto& pt : hole) ar & const_cast<Point&>(pt);
    }
}

template<class Archive, typename Point>
void load(Archive& ar, boost::geometry::model::polygon<Point, false, true>& poly, unsigned) {
    using Polygon = boost::geometry::model::polygon<Point, false, true>;
    poly = Polygon{};

    // exterior
    std::size_t n_outer = 0;
    ar & n_outer;
    auto& outer = poly.outer();
    outer.clear(); outer.reserve(n_outer);
    for (std::size_t i=0;i<n_outer;++i) {
        Point p; ar & p; outer.push_back(p);
    }

    // holes
    std::size_t n_inners = 0;
    ar & n_inners;
    auto& inners = poly.inners();
    inners.clear(); inners.resize(n_inners);
    for (std::size_t h=0; h<n_inners; ++h) {
        std::size_t n=0; ar & n;
        auto& hole = inners[h];
        hole.clear(); hole.reserve(n);
        for (std::size_t i=0;i<n;++i){ Point p; ar & p; hole.push_back(p); }
    }
}
template<class Archive, typename Point>
void serialize(Archive& ar, boost::geometry::model::polygon<Point, false, true>& poly, unsigned version) {
    split_free(ar, poly, version);
}

}} // namespaces
