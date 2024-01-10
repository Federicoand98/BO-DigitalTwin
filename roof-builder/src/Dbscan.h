#pragma once

#include <cassert>
#include <cstddef>
#include <span>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <type_traits>

#include "MyPoint.h"
#include "nanoflann/nanoflann.hpp"

auto dbscan(const std::span<const MyPoint>& data, float eps, int min_pts) -> std::vector<std::vector<size_t>>;
