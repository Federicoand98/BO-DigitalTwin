#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "MyPoint.h"


class Dbscan {

public:
	static std::vector<std::vector<MyPoint>> FindClusters(const std::vector<MyPoint>& points, float epsilon, uint32_t minPoints);
	static void MergeClusters(std::vector<std::vector<MyPoint>>& clusters, float epsilon);

private:
	static std::vector<uint32_t> findNeighbors(const std::vector<MyPoint>& points, int pointIndex, float epsilon);
	static float distance(const MyPoint& p1, const MyPoint& p2);
};


std::vector<std::vector<MyPoint>> Dbscan::FindClusters(const std::vector<MyPoint>& points, float epsilon, uint32_t minPoints) {
	std::vector<std::vector<MyPoint>> clusters;
	int* visited = new int[points.size()];
	std::memset(visited, 0, sizeof(visited));

	for (int i = 0; i < points.size(); i++) {
		if (visited[i] == 1)
			continue;

		visited[i] = 1;

		std::vector<uint32_t> neighbors = findNeighbors(points, i, epsilon);

		if (neighbors.size() < minPoints)
			continue;

		std::vector<MyPoint> cluster;
		cluster.push_back(points.at(i));

		for (uint32_t n : neighbors) {
			if (!visited[n]) {
				visited[n] = 1;
				std::vector<uint32_t> near_n = findNeighbors(points, n, epsilon);

				if(near_n.size() >= minPoints)
					cluster.push_back(points.at(n));
			}
		}

		clusters.push_back(cluster);
	}

	delete[] visited;

	return clusters;
}

void Dbscan::MergeClusters(std::vector<std::vector<MyPoint>>& clusters, float epsilon) {
	bool merged = true;

	while (merged) {
		merged = false;

		for (int i = 0; i < clusters.size(); i++) {
			for (int j = i + 1; j < clusters.size(); j++) {
				if (std::any_of(clusters.at(i).begin(), clusters.at(i).end(), [&](const MyPoint& p1) {
					return std::any_of(clusters.at(j).begin(), clusters.at(j).end(), [&](const MyPoint& p2) {
						return distance(p1, p2) <= epsilon;
					});
				})) {
					std::vector<MyPoint> cluster_j = std::move(clusters.at(j));
					clusters.at(i).insert(clusters.at(i).end(), cluster_j.begin(), cluster_j.end());
					clusters.erase(clusters.begin() + j);
					merged = true;

					break;
				}
			}

			if (merged)
				break;
		}
	}
}

std::vector<uint32_t> Dbscan::findNeighbors(const std::vector<MyPoint>& points, int pointIndex, float epsilon) {
	std::vector<uint32_t> indices;

	std::transform(points.begin(), points.end(), std::back_inserter(indices), [&](const MyPoint& p) {
		return distance(p, points.at(pointIndex)) <= epsilon;
	});

	auto it = std::remove_if(indices.begin(), indices.end(), [](size_t index) {
		return index == 0;
	});

	indices.erase(it, indices.end());

	return indices;
}

float Dbscan::distance(const MyPoint& p1, const MyPoint& p2) {
	return sqrt(pow((p1.x - p2.x), 2) + pow((p1.y - p2.y), 2) + pow((p1.z - p2.z), 2));
}
