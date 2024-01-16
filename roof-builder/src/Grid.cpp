#include "Grid.h"

namespace nf = nanoflann;

void Grid::Init(const std::vector<MyPoint>& inPoints, float csize, float tol, float radius) {
    Clear();
    cellSize = csize;

    minX = std::min_element(inPoints.begin(), inPoints.end(), [](MyPoint const a, MyPoint const b) { return a.x < b.x; })->x - tol;
    maxX = std::max_element(inPoints.begin(), inPoints.end(), [](MyPoint const a, MyPoint const b) { return a.x < b.x; })->x + tol;
    minY = std::min_element(inPoints.begin(), inPoints.end(), [](MyPoint const a, MyPoint const b) { return a.y < b.y; })->y - tol;
    maxY = std::max_element(inPoints.begin(), inPoints.end(), [](MyPoint const a, MyPoint const b) { return a.y < b.y; })->y + tol;

    int num_cells_x = std::ceil((maxX - minX) / cellSize);
    int num_cells_y = std::ceil((maxY - minY) / cellSize);

    points.reserve(num_cells_x * num_cells_y);
    heightMat.resize(num_cells_x, std::vector<float>(num_cells_y, 0.0f));

    PointCloud cloud;
    for (const auto& point : inPoints) {
        cloud.pts.push_back({ point.x, point.y, point.z });
    }

    nf::KDTreeSingleIndexAdaptor<nf::L2_Simple_Adaptor<float, PointCloud>, PointCloud, 2> kdtree
    (2, cloud, nf::KDTreeSingleIndexAdaptorParams(10 /* max leaf */));
    kdtree.buildIndex();


#if MULTITHREADING
#pragma omp parallel for 
    for (int i = 0; i < num_cells_x; ++i) {
        for (int j = 0; j < num_cells_y; ++j) {
            float x = minX + i * cellSize;
            float y = minY + j * cellSize;
            float z = 0.0f;

            const size_t num_results = 1;
            size_t ret_index;
            float out_dist_sqr;
            nf::KNNResultSet<float> resultSet(num_results);
            resultSet.init(&ret_index, &out_dist_sqr);
            float query_pt[2] = { x, y };
            kdtree.findNeighbors(resultSet, &query_pt[0], nf::SearchParams(10));

            if (sqrt(out_dist_sqr) > radius) {
                z = 0.0f;
            }
            else {
                z = cloud.pts[ret_index].z;
            }

            heightMat[i][j] = z;
            if (z > 0) {
#pragma omp critical 
                {
                    points.emplace_back(x, y, z);
                }
            }
        }
    }
#else
    for (int i = 0; i < num_cells_x; ++i) {
        for (int j = 0; j < num_cells_y; ++j) {
            float x = minX + i * cellSize;
            float y = minY + j * cellSize;
            float z = 0.0f;

            const size_t num_results = 1;
            size_t ret_index;
            float out_dist_sqr;
            nf::KNNResultSet<float> resultSet(num_results);
            resultSet.init(&ret_index, &out_dist_sqr);
            float query_pt[2] = { x, y };
            kdtree.findNeighbors(resultSet, &query_pt[0], nf::SearchParams(10));

            if (sqrt(out_dist_sqr) > radius) {
                z = 0.0f;
            }
            else {
                z = cloud.pts[ret_index].z;
            }

            points.emplace_back(x, y, z);
            heightMat[i][j] = z;
        }
    }
#endif
}



void Grid::Clear() {
    minX, maxX, minY, maxY, cellSize = 0;
    points.clear();
    heightMat.clear();
}

void Grid::FillHoles(uint16_t tol, int numIterations) {
    std::vector<std::pair<int, int>> kernel;

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx != 0 || dy != 0) {
                kernel.push_back(std::make_pair(dx, dy));
            }
        }
    }

    int rows = heightMat.size();
    int cols = heightMat[0].size();

    std::vector<std::vector<float>> res = heightMat;

#if MULTITHREADING
 for (int iteration = 0; iteration < numIterations; ++iteration) {
    #pragma omp parallel for
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                if (heightMat[i][j] == 0.0f) {
                    int count = 0;
                    float sum = 0.0;

                    for (auto& [dx, dy] : kernel) {
                        int ni = i + dx;
                        int nj = j + dy;

                        if (ni >= 0 && ni < rows && nj >= 0 && nj < cols) {
                            float neighbour = heightMat[ni][nj];
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
        heightMat = res;
    }
#else
    for (int iteration = 0; iteration < numIterations; ++iteration) {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                if (heightMat[i][j] == 0.0f) {
                    int count = 0;
                    float sum = 0.0;

                    for (auto& [dx, dy] : kernel) {
                        int ni = i + dx;
                        int nj = j + dy;

                        if (ni >= 0 && ni < rows && nj >= 0 && nj < cols) {
                            float neighbour = heightMat[ni][nj];
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
        heightMat = res;
    }
#endif
}


std::vector<std::vector<float>> Grid::GetSobelGradient() {
    std::vector<std::vector<float>> grad(heightMat.size(), std::vector<float>(heightMat[0].size(), 0.0f));

    int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

#if MULTITHREADING
    #pragma omp parallel for
    for (int i = 1; i < heightMat.size() - 1; ++i) {
        for (int j = 1; j < heightMat[i].size() - 1; ++j) {
            float x = (Gx[0][0] * heightMat[i-1][j-1])  + (Gx[0][2] * heightMat[i-1][j+1]) +
                      (Gx[1][0] * heightMat[i][j-1])    + (Gx[1][2] * heightMat[i][j+1]) +
                      (Gx[2][0] * heightMat[i+1][j-1])  + (Gx[2][2] * heightMat[i+1][j+1]);

            float y = (Gy[0][0] * heightMat[i-1][j-1]) + (Gy[0][1] * heightMat[i-1][j]) + (Gy[0][2] * heightMat[i-1][j+1]) +
                      (Gy[2][0] * heightMat[i+1][j-1]) + (Gy[2][1] * heightMat[i+1][j]) + (Gy[2][2] * heightMat[i+1][j+1]);

            grad[i][j] = std::sqrt(x * x + y * y);
        }
    }
#else
    for (int i = 1; i < heightMat.size() - 1; ++i) {
        for (int j = 1; j < heightMat[i].size() - 1; ++j) {
            float x = (Gx[0][0] * heightMat[i-1][j-1])  + (Gx[0][2] * heightMat[i-1][j+1]) +
                      (Gx[1][0] * heightMat[i][j-1])    + (Gx[1][2] * heightMat[i][j+1]) +
                      (Gx[2][0] * heightMat[i+1][j-1])  + (Gx[2][2] * heightMat[i+1][j+1]);

            float y = (Gy[0][0] * heightMat[i-1][j-1]) + (Gy[0][1] * heightMat[i-1][j]) + (Gy[0][2] * heightMat[i-1][j+1]) +
                      (Gy[2][0] * heightMat[i+1][j-1]) + (Gy[2][1] * heightMat[i+1][j]) + (Gy[2][2] * heightMat[i+1][j+1]);

            grad[i][j] = std::sqrt(x * x + y * y);
        }
    }
#endif

    return grad;
}

MyPoint Grid::GetGridPointCoord(int gridX, int gridY) {
    // 0 - 0 in the opencv image is on top left not on bottom left (as in our matrix
    float x = minX + cellSize * gridX;
    float y = maxY - cellSize * gridY;

    PointCloud cloud;
    for (const auto& point : points) {
        cloud.pts.push_back({ point.x, point.y, point.z });
    }


    nf::KDTreeSingleIndexAdaptor<nf::L2_Simple_Adaptor<float, PointCloud>, PointCloud, 2> kdtree
    (2, cloud, nf::KDTreeSingleIndexAdaptorParams(10 /* max leaf */));
    kdtree.buildIndex();

    const size_t num_results = 1;
    size_t ret_index;
    float out_dist_sqr;
    nf::KNNResultSet<float> resultSet(num_results);
    resultSet.init(&ret_index, &out_dist_sqr);
    float query_pt[2] = { x, y };
    kdtree.findNeighbors(resultSet, &query_pt[0], nf::SearchParams(10));

    return cloud.pts[ret_index];
}

std::vector<std::vector<float>> Grid::GetLocalMax(int kernel_size) {
    std::vector<std::vector<float>>& v = heightMat;
    int k = kernel_size / 2;
    std::vector<std::vector<float>> max(v.size(), std::vector<float>(v[0].size(), 0.0));
    std::vector<std::vector<float>> max_check(v.size(), std::vector<float>(v[0].size(), 0.0));

    for (int i = k; i < v.size() - k; ++i) {
        for (int j = k; j < v[i].size() - k; ++j) {
            float temp = 0.0;
            for (int m = -k; m <= k; ++m) {
                for (int n = -k; n <= k; ++n) {
                    int i_index = i + m;
                    int j_index = j + n;

                    float val = v[i_index][j_index];
                    if (val > temp) {
                        temp = val;
                    }
                }
            }
            max[i][j] = temp;
        }
    }

    for (int i = 0; i < v.size(); ++i) {
        for (int j = 0; j < v[i].size(); ++j) {
            if (max[i][j] == v[i][j] && v[i][j] != 0.0) {
                max_check[i][j] = 255.0;
            }
            else {
                max_check[i][j] = 1.0;
            }
        }
    }

    return max_check;
}
