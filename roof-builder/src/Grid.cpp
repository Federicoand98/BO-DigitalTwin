#include "Grid.h"

void Grid::Init(const std::vector<MyPoint>& i_points, float cell_size, float tol, float radius) {
    Clear();

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

void Grid::Clear() {
    min_x, max_x, min_y, max_y, cell_size = 0;
    points.clear();
    height_mat.clear();
}

void Grid::FillHoles(int tol) {
    std::vector<std::pair<int, int>> kernel;

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx != 0 || dy != 0) {
                kernel.push_back(std::make_pair(dx, dy));
            }
        }
    }

    int rows = height_mat.size();
    int cols = height_mat[0].size();

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (height_mat[i][j] == 0.0f) {
                int count = 0;
                float sum = 0.0;

                for (auto& [dx, dy] : kernel) {
                    int ni = i + dx;
                    int nj = j + dy;
                    
                    if (ni >= 0 && ni < rows && nj >= 0 && nj < cols) {
                        float neighbour = height_mat[ni][nj];
                        if (neighbour > 0.0f) {
                            ++count;
                            sum += neighbour;
                        } 
                    }
                }
                if (count > tol) {
                    height_mat[i][j] = sum / static_cast<float>(count);
                }
            }
        }
    }
}