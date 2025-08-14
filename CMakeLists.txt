cmake_minimum_required(VERSION 3.20)
project(tzloc LANGUAGES CXX)

# =========================
# User/options
# =========================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to the GeoJSON file (the actual .json file, not the folder)
if (NOT TZ_JSON)
  message(FATAL_ERROR "Pass -DTZ_JSON=/abs/path/to/timezones.json (GeoJSON)")
endif()

# Optional: build helper tools (packer + embedder)
option(TZLOC_BUILD_TOOLS "Build pack_tzjson and embed_blob tools" ON)

# Disable IPO/LTCG (prevents MSVC 'Generating code…' blowups on huge data)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# =========================
# Dependencies
# =========================
# Prefer modern Boost CONFIG packages; fall back to MODULE (old FindBoost) if needed.
set(_boost_components serialization iostreams)

find_package(Boost QUIET CONFIG COMPONENTS ${_boost_components})
if (NOT Boost_FOUND)
  # Fall back to MODULE mode (legacy FindBoost). CMake >= 3.30 removed it unless policy set.
  if (POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
  endif()
  find_package(Boost REQUIRED COMPONENTS ${_boost_components})
endif()

# Header-only json for the packer tool
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
  DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(nlohmann_json)
set(JSON_HEADER ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/json.hpp)

# =========================
# Tools (packer + embedder)
# =========================
set(PACK_TZJSON_EXE pack_tzjson)
set(EMBED_BLOB_EXE  embed_blob)

if (TZLOC_BUILD_TOOLS)
  add_executable(${PACK_TZJSON_EXE} tools/pack_tzjson.cpp)
  target_include_directories(${PACK_TZJSON_EXE} PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src
  )
  # Link Boost.Serialization for the packer (fixes LNK2019 on MSVC)
  if (TARGET Boost::serialization)
    target_link_libraries(${PACK_TZJSON_EXE} PRIVATE Boost::serialization)
  else()
    target_link_libraries(${PACK_TZJSON_EXE} PRIVATE ${Boost_SERIALIZATION_LIBRARY})
  endif()
  if (MSVC)
    target_compile_options(${PACK_TZJSON_EXE} PRIVATE /bigobj)
  endif()

  add_executable(${EMBED_BLOB_EXE} tools/embed_blob.cpp)
  if (MSVC)
    target_compile_options(${EMBED_BLOB_EXE} PRIVATE /bigobj)
  endif()
endif()

# =========================
# Generated artifacts (ALWAYS in current binary dir)
# =========================
set(GEN_DIR          ${CMAKE_CURRENT_BINARY_DIR})
set(PACKED_BIN       ${GEN_DIR}/tzloc.bin)
set(GENERATED_BLOB   ${GEN_DIR}/generated_blob.cpp)

# Create tzloc.bin from GeoJSON (via packer)
add_custom_command(
  OUTPUT  ${PACKED_BIN}
  COMMAND $<IF:$<BOOL:${TZLOC_BUILD_TOOLS}>,${PACK_TZJSON_EXE},pack_tzjson> ${TZ_JSON} ${PACKED_BIN}
  DEPENDS ${TZ_JSON} ${JSON_HEADER}
  COMMENT "Packing GeoJSON -> ${PACKED_BIN}"
  VERBATIM
)

# Convert tzloc.bin into generated_blob.cpp (embedded byte array)
add_custom_command(
  OUTPUT  ${GENERATED_BLOB}
  COMMAND $<IF:$<BOOL:${TZLOC_BUILD_TOOLS}>,${EMBED_BLOB_EXE},embed_blob> ${PACKED_BIN} ${GENERATED_BLOB}
  DEPENDS ${PACKED_BIN}
  COMMENT "Embedding tzloc.bin -> ${GENERATED_BLOB}"
  VERBATIM
)

# Optional explicit custom target (helps Visual Studio show the generated files)
add_custom_target(tzloc_blob ALL DEPENDS ${GENERATED_BLOB})

# =========================
# Library
# =========================
add_library(timezone_locator
  src/TimezoneLocator.cpp
  ${GENERATED_BLOB}              # <<<< ensures blob TU is compiled into the lib
)

add_dependencies(timezone_locator tzloc_blob)

target_include_directories(timezone_locator PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIRS}
)

# Link Boost.Serialization for the runtime (deserialization)
if (TARGET Boost::serialization)
  target_link_libraries(timezone_locator PRIVATE Boost::serialization)
else()
  target_link_libraries(timezone_locator PRIVATE ${Boost_SERIALIZATION_LIBRARY})
endif()

# MSVC build sanity
if (MSVC)
  target_compile_options(timezone_locator PRIVATE /bigobj)
endif()

# =========================
# Install (optional)
# =========================
install(TARGETS timezone_locator
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
install(FILES include/TimezoneLocator.hpp DESTINATION include)

# =========================
# Developer hints / status
# =========================
message(STATUS "TZ_JSON = ${TZ_JSON}")
message(STATUS "Generated blob will be at: ${GENERATED_BLOB}")
