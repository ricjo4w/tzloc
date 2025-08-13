<p align="center">
  <img src="imgs/tzloc_logo.png" style="width:100px;" />
</p>

# tzloc – Timezone Point-Lookup Library (Boost.Geometry + Boost.Serialization)

`tzloc` answers: **“Which IANA timezone(s) cover this latitude/longitude?”**  
It packs timezone polygons into a **Boost.Serialization binary archive** at build time, embeds that archive directly into the library as a byte array, and **deserializes from memory** at runtime. No runtime file I/O is required.

- **Fast point-in-polygon lookups** with **Boost.Geometry** + **packed R-tree**.
    
- **No giant generated source files** – avoids compiler/linker memory blow-ups.
    
- **Single runtime call**: `TimezoneLocator::instance().query(lat, lon)`.
    

* * *

## Contents

- [Prerequisites](#prerequisites)
    
- [Getting the data](#getting-the-data)
    
- [Build & install](#build--install)
    
- [Using the library in your project](#using-the-library-in-your-project)
    
- [API](#api)
    
- [Examples](#examples)
    
- [Notes on performance & memory](#notes-on-performance--memory)
    
- [Build pipeline](#build-pipeline)
    
- [Troubleshooting](#troubleshooting)
    
- [License](#license)
    

* * *

## Prerequisites

- **C++17** or newer
    
- **CMake ≥ 3.20**
    
- **Boost ≥ 1.74** (header-only Geometry, Serialization, and iostreams components)
    
- A modern compiler (GCC 9+/Clang 10+/MSVC 2019+)
    

> This project also uses **nlohmann/json** (header-only) for the build-time packer. CMake will fetch it automatically; you don’t need to install it.

* * *

## Getting the data

This project expects a GeoJSON file from the **timezone-boundary-builder** project.

1.  Download from: https://github.com/evansiroky/timezone-boundary-builder
    
2.  Use the full-feature GeoJSON file (e.g., `timezones.geojson`).
    

You will pass the absolute path to this file via `-DTZ_JSON=...` when configuring.

* * *

## Build & install

```bash
# From the repository root
cmake -S . -B build -DTZ_JSON=/abs/path/to/timezones.geojson -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The build process will:

1.  **Run the packer tool** to parse the GeoJSON into `tzloc.bin` (Boost.Serialization binary archive).
    
2.  **Run the embedder tool** to convert `tzloc.bin` into `generated_blob.cpp` (a `const unsigned char[]`).
    
3.  **Compile** the runtime library with the embedded blob.
    
4.  At runtime, the library **deserializes from memory** and builds the R-tree.
    

### Optional: install

```bash
cmake --install build --prefix /your/prefix
```

Installs:

- `libtimezone_locator.*` → `/your/prefix/lib`
    
- `include/TimezoneLocator.hpp` → `/your/prefix/include`
    

* * *

## Using the library in your project

### Vendored (add_subdirectory)

If `tzloc/` is part of your source tree:

```cmake
add_subdirectory(external/tzloc)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE timezone_locator)
target_include_directories(app PRIVATE external/tzloc/include)
```

Configure **your** project with the same `-DTZ_JSON` path so the packer runs:

```bash
cmake -S . -B build -DTZ_JSON=/abs/path/to/timezones.geojson
cmake --build build
```

* * *

## API

```cpp
namespace tzloc {

class TimezoneLocator {
public:
    static const TimezoneLocator& instance();

    std::vector<std::string> query(double latitude,
                                   double longitude,
                                   bool include_boundary = true) const;

    std::vector<std::string> queryLonLat(double longitude,
                                         double latitude,
                                         bool include_boundary = true) const;
};

} // namespace tzloc
```

- **`include_boundary=true`** → points on polygon borders count as inside.
    
- Returns all matching tzids (deduplicated) if polygons overlap.
    
- Index is built once on first `instance()` call.
    

* * *

## Examples

**Minimal usage:**

```cpp
#include <iostream>
#include "TimezoneLocator.hpp"

int main() {
    auto& locator = tzloc::TimezoneLocator::instance();
    auto tzs = locator.query(40.7580, -73.9855); // Times Square
    for (auto& t : tzs)
        std::cout << t << "\n";
}
```

**CMake for the example (vendored):**

```cmake
cmake_minimum_required(VERSION 3.20)
project(example LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(../tzloc tzloc-build)
add_executable(example main.cpp)
target_link_libraries(example PRIVATE timezone_locator)
target_include_directories(example PRIVATE ../tzloc/include)
```

* * *

## Notes on performance & memory

- **Startup**: Deserialization from the embedded Boost.Serialization archive is **much faster** than parsing JSON.
    
- **Memory**: Uses heap storage for deserialized vectors of polygons and tzids. Slightly higher RAM than pure codegen arrays, but avoids multi-GB compiler memory use.
    
- **Binary size**: The embedded blob is roughly the size of the serialized data. Can be compressed if desired.
    
- **Query speed**: Identical to the codegen approach — packed R-tree + Boost.Geometry point-in-polygon.
    

* * *

## Build pipeline

```
+--------------------------+
| timezones.geojson (IANA) |
+--------------------------+
              |
              v
    +------------------+
    | pack_tzjson tool |  (parses GeoJSON, serializes with Boost.Serialization)
    +------------------+
              |
              v
+---------------------------+
| tzloc.bin (binary archive)|
+---------------------------+
              |
              v
    +-------------------+
    | embed_blob tool   |  (generates generated_blob.cpp with const unsigned char[])
    +-------------------+
              |
              v
+--------------------------------------+
| timezone_locator library build       |
|  - Compiles generated_blob.cpp       |
|  - Includes TimezoneLocator.cpp      |
+--------------------------------------+
              |
              v
+--------------------------------------+
| Runtime: TimezoneLocator::instance() |
|  - Deserializes from embedded blob   |
|  - Builds packed R-tree              |
|  - Serves queries instantly          |
+--------------------------------------+
```

* * *

## Troubleshooting

**CMake errors about `TZ_JSON`**  
Pass the file path when configuring:

```bash
cmake -S . -B build -DTZ_JSON=/abs/path/to/timezones.geojson
```

**Wrong results**  
Check coordinate ordering. `query(lat, lon)` expects latitude first, longitude second.

* * *

## License

- **Library code**: MIT.
    
- **Timezone geometry**: Provided by *timezone-boundary-builder*, which has its own license and attribution terms. Comply with their requirements when distributing binaries that include this data.
