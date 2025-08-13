#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include <nlohmann/json.hpp>
using nlohmann::json;

#include <boost/archive/binary_oarchive.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include "../src/serialization.hpp"

namespace bg = boost::geometry;

using Point   = bg::model::d2::point_xy<double>;
using Polygon = bg::model::polygon<Point, /*Clockwise?*/ false, /*Closed?*/ true>;

static void add_ring(Polygon& poly, const json& ring) {
    auto& outer = poly.outer();
    outer.clear();
    if (!ring.is_array() || ring.size() < 3) return;
    const double lon0 = ring.at(0).at(0).get<double>();
    const double lat0 = ring.at(0).at(1).get<double>();
    const double lonN = ring.back().at(0).get<double>();
    const double latN = ring.back().at(1).get<double>();
    const bool closed = (lon0==lonN && lat0==latN);
    const size_t end = closed ? ring.size()-1 : ring.size();
    outer.reserve(end+1);
    for (size_t i=0;i<end;++i)
        outer.emplace_back(ring.at(i).at(0).get<double>(), ring.at(i).at(1).get<double>());
    outer.emplace_back(lon0, lat0);
}

static void add_holes(Polygon& poly, const json& rings) {
    auto& inners = poly.inners();
    for (size_t r=1; r<rings.size(); ++r) {
        const auto& ring = rings.at(r);
        if (!ring.is_array() || ring.size() < 3) continue;
        const double lon0 = ring.at(0).at(0).get<double>();
        const double lat0 = ring.at(0).at(1).get<double>();
        const double lonN = ring.back().at(0).get<double>();
        const double latN = ring.back().at(1).get<double>();
        const bool closed = (lon0==lonN && lat0==latN);
        const size_t end = closed ? ring.size()-1 : ring.size();

        inners.emplace_back();
        auto& hole = inners.back();
        hole.reserve(end+1);
        for (size_t i=0;i<end;++i)
            hole.emplace_back(ring.at(i).at(0).get<double>(), ring.at(i).at(1).get<double>());
        hole.emplace_back(lon0, lat0);
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: pack_tzjson <input_geojson> <output_bin>\n";
        return 2;
    }
    const std::string in_path  = argv[1];
    const std::string out_path = argv[2];

    std::ifstream ifs(in_path);
    if (!ifs) { std::cerr << "ERROR: cannot open " << in_path << "\n"; return 1; }

    json j; ifs >> j;
    if (!j.contains("features") || !j["features"].is_array()) {
        std::cerr << "ERROR: not a FeatureCollection\n"; return 1;
    }

    std::vector<Polygon> polys;
    std::vector<std::string> tzids;
    polys.reserve(20000); tzids.reserve(20000);

    for (const auto& f : j["features"]) {
        const auto& props = f["properties"];
        std::string tzid;
        if (props.contains("tzid")) tzid = props["tzid"].get<std::string>();
        else if (props.contains("TZID")) tzid = props["TZID"].get<std::string>();
        else if (props.contains("name")) tzid = props["name"].get<std::string>();
        if (tzid.empty()) continue;

        const auto& geom = f["geometry"];
        const std::string type = geom["type"].get<std::string>();

        if (type == "Polygon") {
            const auto& rings = geom["coordinates"];
            Polygon p;
            add_ring(p, rings.at(0));
            add_holes(p, rings);
            bg::correct(p);
            polys.push_back(std::move(p));
            tzids.push_back(tzid);

        } else if (type == "MultiPolygon") {
            const auto& polys_geo = geom["coordinates"];
            for (const auto& rings : polys_geo) {
                Polygon p;
                add_ring(p, rings.at(0));
                add_holes(p, rings);
                bg::correct(p);
                polys.push_back(std::move(p));
                tzids.push_back(tzid);
            }
        }
    }

    std::ofstream ofs(out_path, std::ios::binary|std::ios::trunc);
    if (!ofs) { std::cerr << "ERROR: cannot open " << out_path << " for write\n"; return 1; }
    boost::archive::binary_oarchive oa(ofs);
    oa & polys;
    oa & tzids;

    std::cerr << "Packed " << polys.size() << " polygons to " << out_path << "\n";
    return 0;
}
