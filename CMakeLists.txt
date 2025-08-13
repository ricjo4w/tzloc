cmake_minimum_required(VERSION 3.20)
project(tzloc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Input GeoJSON (required) ----
if (NOT TZ_JSON)
  message(FATAL_ERROR "Pass -DTZ_JSON=/abs/path/to/timezones.geojson")
endif()

# ---- Dependencies ----
find_package(Boost 1.74 REQUIRED)   # header-only geometry is enough

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
  DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(nlohmann_json)
set(JSON_HEADER ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/json.hpp)

# ---- Tools ----
add_executable(pack_tzjson tools/pack_tzjson.cpp)
target_include_directories(pack_tzjson PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(pack_tzjson PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(pack_tzjson PRIVATE ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src)

add_executable(embed_blob tools/embed_blob.cpp)

# ---- Generated artifacts ----
set(PACKED_BIN ${CMAKE_BINARY_DIR}/tzloc.bin)
add_custom_command(
  OUTPUT ${PACKED_BIN}
  COMMAND pack_tzjson ${TZ_JSON} ${PACKED_BIN}
  DEPENDS pack_tzjson ${TZ_JSON} ${JSON_HEADER}
  COMMENT "Packing GeoJSON -> ${PACKED_BIN}"
  VERBATIM)

set(GENERATED_BLOB ${CMAKE_BINARY_DIR}/generated_blob.cpp)
add_custom_command(
  OUTPUT ${GENERATED_BLOB}
  COMMAND embed_blob ${PACKED_BIN} ${GENERATED_BLOB}
  DEPENDS embed_blob ${PACKED_BIN}
  COMMENT "Embedding tzloc.bin -> ${GENERATED_BLOB}"
  VERBATIM)

# ---- Library ----
add_library(timezone_locator
  src/TimezoneLocator.cpp
  ${GENERATED_BLOB}
)
target_include_directories(timezone_locator PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIRS}
)

# MSVC: Avoid LTCG & allow big objs
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
if (MSVC)
  target_compile_options(timezone_locator PRIVATE /bigobj)
  target_compile_options(pack_tzjson      PRIVATE /bigobj)
endif()

# ---- Install (optional) ----
install(TARGETS timezone_locator
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
install(FILES include/TimezoneLocator.hpp DESTINATION include)
