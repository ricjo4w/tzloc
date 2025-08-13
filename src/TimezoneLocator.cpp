#include "TimezoneLocator.hpp"
#include "serialization.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace tzloc_blob_ns {
// provided by generated_blob.cpp
extern const unsigned char tzloc_blob[];
extern const std::size_t   tzloc_blob_size;
}

namespace tzloc {

Box TimezoneLocator::bboxOf(const Polygon& poly) {
    Box b;
    bg::envelope(poly, b);
    return b;
}

const TimezoneLocator& TimezoneLocator::instance() {
    static const TimezoneLocator inst;
    return inst;
}

TimezoneLocator::TimezoneLocator() {
    // 1) Deserialize from embedded blob
    namespace bio = boost::iostreams;
    bio::array_source src(reinterpret_cast<const char*>(tzloc_blob_ns::tzloc_blob),
                          tzloc_blob_ns::tzloc_blob_size);
    bio::stream<bio::array_source> is(src);
    boost::archive::binary_iarchive ia(is);

    ia & polys_;
    ia & tzids_;

    // Optional: correct ring orientation/closure (safety net)
    for (auto& p : polys_) boost::geometry::correct(p);

    // 2) Build packed R-tree
    std::vector<std::pair<Box,int>> entries;
    entries.reserve(polys_.size());
    for (int i = 0; i < static_cast<int>(polys_.size()); ++i)
        entries.emplace_back(bboxOf(polys_[i]), i);
    rtree_ = RTree(entries.begin(), entries.end());
}

std::vector<std::string> TimezoneLocator::query(double latitude,
                                                double longitude,
                                                bool include_boundary) const
{
    auto normLon = [](double lon) {
        double x = std::fmod(lon + 180.0, 360.0);
        if (x < 0) x += 360.0;
        return x - 180.0;
    };
    const Point pt{normLon(longitude), latitude};

    std::vector<std::pair<Box,int>> candidates;
    rtree_.query(bgi::contains(pt), std::back_inserter(candidates));

    std::vector<std::string> hits;
    hits.reserve(candidates.size());

    for (auto [box, idx] : candidates) {
        const auto& poly = polys_[idx];
        bool inside = include_boundary ? bg::covered_by(pt, poly)
                                       : bg::within(pt, poly);
        if (inside) hits.push_back(tzids_[idx]);
    }
    std::sort(hits.begin(), hits.end());
    hits.erase(std::unique(hits.begin(), hits.end()), hits.end());
    return hits;
}

} // namespace tzloc
