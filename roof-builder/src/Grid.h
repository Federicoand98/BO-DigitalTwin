#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include "MyPoint.h"

class Grid {
public:
    float min_x, max_x, min_y, max_y;
    float cell_size;
    std::vector<MyPoint> points;
    std::vector<std::vector<float>> height_mat;

    Grid(const std::vector<MyPoint>& i_points, float cell_size, float tol, float radius) {
        min_x = std::min_element(i_points.begin(), i_points.end(), [] (MyPoint const a, MyPoint const b) { return a.x < b.x; })->x - tol;
        max_x = std::max_element(i_points.begin(), i_points.end(), [] (MyPoint const a, MyPoint const b) { return a.x < b.x; })->x + tol;
        min_y = std::min_element(i_points.begin(), i_points.end(), [] (MyPoint const a, MyPoint const b) { return a.y < b.y; })->y - tol;
        max_y = std::max_element(i_points.begin(), i_points.end(), [] (MyPoint const a, MyPoint const b) { return a.y < b.y; })->y + tol;

        int num_cells_x = std::ceil((max_x - min_x) / cell_size);
        int num_cells_y = std::ceil((max_y - min_y) / cell_size);

        points.reserve(num_cells_x * num_cells_y);
        height_mat.resize(num_cells_x, std::vector<float>(num_cells_y, 0.0f));

        for (int i = 0; i < num_cells_x; ++i) {
            for (int j = 0; j < num_cells_y; ++j) {
                float x = min_x + i * cell_size;
                float y = min_y + j * cell_size;
                float z = 0.0f;

                float min_distance = radius;
                float closest_z = 0.0f;
                for (const MyPoint& point : i_points) {
                    float distance = point.distance_xy(MyPoint(x, y, 0.0f));
                    if (distance < min_distance) {
                        min_distance = distance;
                        closest_z = point.z;
                        if (distance <= 0.02f) {
                            break;
                        }
                    }
                }
                z = closest_z;

                points.emplace_back(x, y, z);
                height_mat[i][j] = z;
            }
        }
    }

    float get_min_x() const { return min_x; }
    float get_max_x() const { return max_x; }
    float get_min_y() const { return min_y; }
    float get_max_y() const { return max_y; }
    float get_cell_size() const { return cell_size; }
    const std::vector<MyPoint>& get_points() const { return points; }
    const std::vector<std::vector<float>>& get_height_mat() const { return height_mat; }
};
