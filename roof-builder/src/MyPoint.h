#pragma once
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>

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

    MyPoint cross(const MyPoint& other) const {
        return MyPoint(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }

    MyPoint normalize() const {
        float magnitude = std::sqrt(x * x + y * y + z * z);
        return MyPoint(x / magnitude, y / magnitude, z / magnitude);
    }

    friend std::ostream& operator<<(std::ostream& os, const MyPoint& point) {
        os << std::fixed << std::setprecision(3);
        os << "MyPoint(" << point.x << ", " << point.y << ", " << point.z << ")";
        return os;
    }

    MyPoint operator-(const MyPoint& other) const {
        return MyPoint(x - other.x, y - other.y, z - other.z);
    }
};

struct PointCloud {
    std::vector<MyPoint>  pts;

    inline size_t kdtree_get_point_count() const { return pts.size(); }

    inline float kdtree_distance(const float* p1, const size_t idx_p2, size_t /*size*/) const {
        const float d0 = p1[0] - pts[idx_p2].x;
        const float d1 = p1[1] - pts[idx_p2].y;
        return d0 * d0 + d1 * d1;
    }

    inline float kdtree_get_pt(const size_t idx, int dim) const {
        if (dim == 0) return pts[idx].x;
        else return pts[idx].y;
    }

    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }
};

