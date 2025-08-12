cmake_minimum_required(VERSION 3.20)
project(tzloc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- User input: path to your timezone GeoJSON (from timezone-boundary-builder) ----
if (NOT TZ_JSON)
  message(FATAL_ERROR "Please pass -DTZ_JSON=/absolute/path/to/timezones.geojson")
endif()

# ---- Dependencies ----
# Boost.Geometry is header-only; but we still request Boost to get include dirs if needed.
find_package(Boost 1.74 REQUIRED)

# nlohmann_json for the generator (header-only); FetchContent keeps consumers clean.
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
  DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Place the single header where the generator can include it
set(JSON_HEADER ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/json.hpp)

# ---- Generator ----
add_executable(gen_tz tools/gen_tz.cpp)
target_include_directories(gen_tz PRIVATE ${Boost_INCLUDE_DIRS})
# Make the single-header visible via the include path
target_include_directories(gen_tz PRIVATE ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src)

# Custom step: run generator -> produced source file
set(GENERATED_CPP ${CMAKE_BINARY_DIR}/generated_tz_data.cpp)
add_custom_command(
  OUTPUT ${GENERATED_CPP}
  COMMAND gen_tz ${TZ_JSON} ${GENERATED_CPP}
  DEPENDS gen_tz ${TZ_JSON} ${JSON_HEADER}
  COMMENT "Generating baked timezone data translation unit"
  VERBATIM)

# ---- Library ----
add_library(timezone_locator
  src/TimezoneLocator.cpp
  ${GENERATED_CPP}
)
target_include_directories(timezone_locator PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIRS}
)
# No link libs needed (Boost.Geometry header-only)
# You may set position independent code for shared builds:
set_target_properties(timezone_locator PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- Install (optional) ----
install(TARGETS timezone_locator
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
install(FILES include/TimezoneLocator.hpp DESTINATION include)
