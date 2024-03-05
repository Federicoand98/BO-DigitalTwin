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


class Dbscan {
public:
	static std::vector<MyPoint> GetMainCluster(const std::span<const MyPoint>& points, float eps, int minPts) {
		std::vector<std::vector<size_t>> clusters = dbscan(points, eps, minPts);


		size_t largest_cluster_idx = 0;
		size_t max_size = 0;

		for (size_t i = 0; i < clusters.size(); ++i) {
			if (clusters[i].size() > max_size) {
				max_size = clusters[i].size();
				largest_cluster_idx = i;
			}
		}

		std::vector<MyPoint> largest_cluster;
		if (max_size != 0) {
			for (auto idx : clusters[largest_cluster_idx]) {
				largest_cluster.push_back(points[idx]);
			}
		}

		return largest_cluster;
	}
};