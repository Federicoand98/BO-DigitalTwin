#pragma once
#include <cmath>

class MyPoint {
public:
    float x, y, z;

    MyPoint(float x, float y, float z) : x(x), y(y), z(z) {}

    float distance_xy(const MyPoint& other) const {
        return std::sqrt(std::pow(other.x - x, 2) + std::pow(other.y - y, 2));
    }

    float distance(const MyPoint& other) const {
        return std::sqrt(std::pow(other.x - x, 2) + std::pow(other.y - y, 2) + std::pow(other.z - z, 2));
    }
};

