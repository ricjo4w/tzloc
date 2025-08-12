<p align="center">
  <img src="imgs/tzloc_logo.png" style="width: 200px" />
</p>

# tzloc – Timezone point-lookup library (Boost.Geometry + R-tree)

`tzloc` is a small C++ library that answers: **“Which IANA timezone(s) cover this latitude/longitude?”**  
It bakes the timezone polygons directly into the library at **build time**, so your apps do **zero file I/O** at runtime.

- Fast point-in-polygon using **Boost.Geometry** with a packed **R-tree** broad-phase.
    
- Supports **overlapping polygons** (returns all matches).
    
- Simple API: `TimezoneLocator::instance().query(lat, lon)` → `std::vector<std::string>`.
    

* * *

## Contents

- [Prerequisites](#prerequisites)
    
- [Getting the data](#getting-the-data)
    
- [Build & install](#build--install)
    
- [Using the library in your project](#using-the-library-in-your-project)
    
- [API](#api)
    
- [Examples](#examples)
    
- [Notes on accuracy & performance](#notes-on-accuracy--performance)
    
- [Troubleshooting](#troubleshooting)
    
- [License](#license)
    

* * *

## Prerequisites

- **C++17** or newer
    
- **CMake ≥ 3.20**
    
- **Boost ≥ 1.74** (header-only for Geometry; ensure headers are available)
    
- A modern compiler (GCC 9+/Clang 10+/MSVC 2019+)
    

> The build uses a small generator (`tools/gen_tz.cpp`) that depends on the single-header **nlohmann/json**. CMake will fetch it automatically; you don’t need to install anything.

* * *

## Getting the data

This project expects a GeoJSON file from the excellent **timezone-boundary-builder** project.

1.  Download a release from: https://github.com/evansiroky/timezone-boundary-builder
    
2.  Use the **GeoJSON** file that contains all features (e.g., `timezones.geojson`).
    

You will pass the absolute path to this file via `-DTZ_JSON=...` at configure time.

* * *

## Build & install

```bash
# From the repository root
cmake -S . -B build -DTZ_JSON=/abs/path/to/timezones.geojson -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

This does three things:

1.  **Builds the generator** (`gen_tz`).
    
2.  **Runs** the generator on your GeoJSON to produce `generated_tz_data.cpp` (in the build tree).
    
3.  **Builds** the `timezone_locator` library by compiling the runtime sources **plus** the generated TU.
    

### Optional: install

```bash
cmake --install build --prefix /your/prefix
```

This installs:

- `libtimezone_locator.*` to `/your/prefix/lib`
    
- `include/TimezoneLocator.hpp` to `/your/prefix/include`
    

> Note: This project installs headers/libs but does not ship a CMake package config yet. See “Using the library” for integration options.

* * *

## Using the library in your project

You have two common options.

### Option A — Vendored (add_subdirectory)

If `tzloc/` is present in your source tree:

```cmake
# Your app's CMakeLists.txt
add_subdirectory(external/tzloc)         # path to this repo
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE timezone_locator)
target_include_directories(app PRIVATE external/tzloc/include)
```

Configure **your** project with the same `-DTZ_JSON=...` flag so the generator runs during your build:

```bash
cmake -S . -B build -DTZ_JSON=/abs/path/to/timezones.geojson
cmake --build build
```

### Option B — Installed system-wide (or prefix)

If you ran `cmake --install`, you can link to the installed library:

```cmake
# Minimal example without a package config:
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE timezone_locator)
target_include_directories(app PRIVATE /your/prefix/include)
link_directories(/your/prefix/lib)  # or use -DCMAKE_PREFIX_PATH to avoid this
```

* * *

## API

```cpp
namespace tzloc {

class TimezoneLocator {
public:
    // Thread-safe singleton
    static const TimezoneLocator& instance();

    // Query by (lat, lon). Returns all covering timezone names (tzids).
    std::vector<std::string> query(double latitude,
                                   double longitude,
                                   bool include_boundary = true) const;

    // Alternative overload if you prefer (lon, lat) ordering.
    std::vector<std::string> queryLonLat(double longitude,
                                         double latitude,
                                         bool include_boundary = true) const;
};

} // namespace tzloc
```

- **`include_boundary=true`** counts points on polygon borders as inside (`covered_by`); set to `false` for strict interior (`within`).
    
- Multiple tzids can be returned if polygons overlap.
    
- Construction of the internal index happens **once** on first `instance()` call.
    

* * *

## Examples

### Minimal program

```cpp
#include <iostream>
#include "TimezoneLocator.hpp"

int main() {
    // Times Square, NYC
    double lat = 40.7580;
    double lon = -73.9855;

    const auto& locator = tzloc::TimezoneLocator::instance();
    auto tzs = locator.query(lat, lon);

    if (tzs.empty()) {
        std::cout << "No timezone found\n";
    } else {
        for (const auto& id : tzs) std::cout << id << "\n";
    }
}
```

### CMake for the example (vendored)

```cmake
cmake_minimum_required(VERSION 3.20)
project(example LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(../tzloc tzloc-build)  # path to this repo
add_executable(example main.cpp)
target_link_libraries(example PRIVATE timezone_locator)
target_include_directories(example PRIVATE ../tzloc/include)
```

Build:

```bash
cmake -S . -B build -DTZ_JSON=/abs/path/to/timezones.geojson
cmake --build build
./build/example
```

* * *

## Notes on accuracy & performance

- **Geometry & coordinates**: The library treats coordinates as **planar** `(x=lon, y=lat)` using Boost.Geometry Cartesian types. For timezone boundaries this is a common, pragmatic choice that performs very well. If you require geodesic semantics, the code can be adapted to `bg::cs::geographic<bg::degree>` with proper strategies (slower).
    
- **R-tree**: The R-tree is built with the packed constructor (quadratic, 16 entries per node) for **fast queries** on static data.
    
- **Overlaps**: Some features may overlap (e.g., disputed areas or micro-polygons). The API returns **all** matching tzids (deduplicated).
    
- **Dateline**: The dataset generally avoids pathological antimeridian wraps. If you plan to query points exactly on ±180°, consider using `include_boundary=false` or adding special handling.
    

* * *

## Troubleshooting

**CMake errors about `TZ_JSON`**  
You must pass the path at configure time, e.g.:

```bash
cmake -S . -B build -DTZ_JSON=/abs/path/to/timezones.geojson
```

**Generator can’t find `json.hpp`**  
CMake fetches the header automatically. Ensure your machine can download from GitHub during the first configure, or vendor `json.hpp` and adjust `CMakeLists.txt`.

**Object file too large / slow compile**  
The generated TU can be big. Solutions:

- Build in **Release**.
    
- Enable compiler **/bigobj** (MSVC) or increase section limits.
    
- Shard the generated file into multiple `.cpp`s (the generator can be extended to do this).
    

**No results for a valid point**

- Verify `(lat, lon)` order in your call. `query(lat, lon)` expects **latitude then longitude**.
    
- Try `include_boundary=false` to avoid boundary ambiguities.
    

* * *

## License

- Library source in this repository: MIT.
    
- The timezone geometry comes from **timezone-boundary-builder**, which has its own license/attribution terms. Ensure you comply with their project’s license when distributing binaries that embed the data.
