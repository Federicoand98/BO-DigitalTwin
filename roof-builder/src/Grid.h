#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <omp.h>
#include "MyPoint.h"

class Grid {
public:
    Grid();
    void Init(const std::vector<MyPoint>& i_points, float cell_size, float tol, float radius);
    void Clear();

    void FillHoles(uint16_t tol);
    std::vector<std::vector<float>> GetSobelGradient();
    MyPoint GetGridPointCoord(int grid_x, int grid_y);

    float GetMinX() const { return min_x; }
    float GetMaxX() const { return max_x; }
    float GetMinY() const { return min_y; }
    float GetMaxY() const { return max_y; }
    float GetCellSize() const { return cell_size; }
    const std::vector<MyPoint>& GetPoints() const { return points; }
    const std::vector<std::vector<float>>& GetHeightMat() const { return height_mat; }

private:
    float min_x, max_x, min_y, max_y;
    float cell_size;
    std::vector<MyPoint> points;
    std::vector<std::vector<float>> height_mat;
};
