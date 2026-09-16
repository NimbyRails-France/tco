#pragma once
#include <cstdint>
// Geometry owned by the TCO, independent of SDK record layout.
struct MapNode { std::uint64_t id, link_a, link_b; double x, y; };
