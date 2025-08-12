#include "TimezoneLocator.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// This symbol is provided by the generated file at build time.
namespace tzdb_gen {
    // Fills out vectors with all polygons and their tzids (1:1).
    // Implemented in generated_tz_data.cpp (auto-generated at build time).
    void build(std::vector<tzloc::Polygon>& out_polys,
               std::vector<std::string>& out_tzids);
}

namespace tzloc {

Box TimezoneLocator::bboxOf(const Polygon& poly) {
    Box box;
    bg::envelope(poly, box);
    return box;
}

const TimezoneLocator& TimezoneLocator::instance() {
    static const TimezoneLocator inst; // thread-safe in C++11+
    return inst;
}

TimezoneLocator::TimezoneLocator() {
    // Load baked data (no file I/O)
    tzdb_gen::build(polys_, tzids_);

    // Build packed R-tree from bounding boxes
    std::vector<std::pair<Box, int>> entries;
    entries.reserve(polys_.size());
    for (int i = 0; i < static_cast<int>(polys_.size()); ++i)
        entries.emplace_back(bboxOf(polys_[i]), i);

    rtree_ = RTree(entries.begin(), entries.end());
}

std::vector<std::string> TimezoneLocator::query(double latitude,
                                                double longitude,
                                                bool include_boundary) const
{
    // Normalize lon a bit, just in case input is outside [-180,180]
    auto normLon = [](double lon) {
        // wrap to (-180, 180]
        double x = std::fmod(lon + 180.0, 360.0);
        if (x < 0) x += 360.0;
        return x - 180.0;
    };

    const Point pt{normLon(longitude), latitude};

    // Broad-phase: AABB filter via "contains" on boxes
    std::vector<std::pair<Box,int>> candidates;
    rtree_.query(bgi::contains(pt), std::back_inserter(candidates));

    std::vector<std::string> hits;
    hits.reserve(candidates.size());

    for (auto [box, idx] : candidates) {
        const auto& poly = polys_[idx];

        // covered_by counts boundary as inside; within is strict interior.
        const bool inside = include_boundary
            ? bg::covered_by(pt, poly)
            : bg::within(pt, poly);

        if (inside)
            hits.push_back(tzids_[idx]);
    }

    // Optional: deduplicate (in case multiple disjoint parts share the same tzid)
    std::sort(hits.begin(), hits.end());
    hits.erase(std::unique(hits.begin(), hits.end()), hits.end());
    return hits;
}

} // namespace tzloc
