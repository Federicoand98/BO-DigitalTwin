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

#if MULTITHREADING
    #pragma omp parallel for
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

            #pragma omp critical 
            {
                points.emplace_back(x, y, z);
                height_mat[i][j] = z;
            }
        }
    }
#else
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
#endif
}

void Grid::Clear() {
    min_x, max_x, min_y, max_y, cell_size = 0;
    points.clear();
    height_mat.clear();
}

void Grid::FillHoles(uint16_t tol) {
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

    std::vector<std::vector<float>> res = height_mat;

#if MULTITHREADING
    #pragma omp parallel for
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
                    res[i][j] = sum / static_cast<float>(count);
                }
            }
        }
    }
#else
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
                    res[i][j] = sum / static_cast<float>(count);
                }
            }
        }
    }
#endif
    height_mat = res;
}

std::vector<std::vector<float>> Grid::GetSobelGradient() {
    std::vector<std::vector<float>> grad(height_mat.size(), std::vector<float>(height_mat[0].size(), 0.0f));

    int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

#if MULTITHREADING
    #pragma omp parallel for
    for (int i = 1; i < height_mat.size() - 1; ++i) {
        for (int j = 1; j < height_mat[i].size() - 1; ++j) {
            float x = (Gx[0][0] * height_mat[i-1][j-1])  + (Gx[0][2] * height_mat[i-1][j+1]) +
                      (Gx[1][0] * height_mat[i][j-1])    + (Gx[1][2] * height_mat[i][j+1]) +
                      (Gx[2][0] * height_mat[i+1][j-1])  + (Gx[2][2] * height_mat[i+1][j+1]);

            float y = (Gy[0][0] * height_mat[i-1][j-1]) + (Gy[0][1] * height_mat[i-1][j]) + (Gy[0][2] * height_mat[i-1][j+1]) +
                      (Gy[2][0] * height_mat[i+1][j-1]) + (Gy[2][1] * height_mat[i+1][j]) + (Gy[2][2] * height_mat[i+1][j+1]);

            grad[i][j] = std::sqrt(x * x + y * y);
        }
    }
#else
    for (int i = 1; i < height_mat.size() - 1; ++i) {
        for (int j = 1; j < height_mat[i].size() - 1; ++j) {
            float x = (Gx[0][0] * height_mat[i-1][j-1])  + (Gx[0][2] * height_mat[i-1][j+1]) +
                      (Gx[1][0] * height_mat[i][j-1])    + (Gx[1][2] * height_mat[i][j+1]) +
                      (Gx[2][0] * height_mat[i+1][j-1])  + (Gx[2][2] * height_mat[i+1][j+1]);

            float y = (Gy[0][0] * height_mat[i-1][j-1]) + (Gy[0][1] * height_mat[i-1][j]) + (Gy[0][2] * height_mat[i-1][j+1]) +
                      (Gy[2][0] * height_mat[i+1][j-1]) + (Gy[2][1] * height_mat[i+1][j]) + (Gy[2][2] * height_mat[i+1][j+1]);

            grad[i][j] = std::sqrt(x * x + y * y);
        }
    }
#endif

    return grad;
}

MyPoint Grid::GetGridPointCoord(int grid_x, int grid_y) {
    float x = min_x + cell_size * grid_x;
    float y = min_y + cell_size * grid_y;

    float min_distance = 100.0f;
    float closest_z = 0.0f;
    for (const MyPoint& point : points) {
        float distance = point.distance_xy(MyPoint(x, y, 0.0f));
        if (distance < min_distance && point.z > 0.0f) {
            min_distance = distance;
            closest_z = point.z;
            if (min_distance <= 0.02f) {
                break;
            }
        }
    }

    return MyPoint(x, y, closest_z);
}