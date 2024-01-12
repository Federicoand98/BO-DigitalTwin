#pragma once

#include <iostream> 
#include <vector>
#include <algorithm>
#include <cmath>
#include <omp.h>
#include "MyPoint.h"
#include "nanoflann/nanoflann.hpp"

class Grid {
public:
    Grid() {};
    void Init(const std::vector<MyPoint>& i_points, float cell_size, float tol, float radius);
    void Clear();

    void FillHoles(uint16_t tol, int numIterations = 1);
    std::vector<std::vector<float>> GetSobelGradient();
    MyPoint GetGridPointCoord(int grid_x, int grid_y);

    float GetMinX() const { return minX; }
    float GetMaxX() const { return maxX; }
    float GetMinY() const { return minY; }
    float GetMaxY() const { return maxY; }
    float GetCellSize() const { return cellSize; }
    const std::vector<MyPoint>& GetPoints() const { return points; }
    const std::vector<std::vector<float>>& GetHeightMat() const { return heightMat; }

private:
    float minX, maxX, minY, maxY;
    float cellSize;
    std::vector<MyPoint> points;
    std::vector<std::vector<float>> heightMat;
};
