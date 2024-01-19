#pragma once
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

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
        os << point.x << " " << point.y << " " << point.z;
        return os;
    }

    std::string toString() const {
        std::ostringstream os;
        os << std::fixed << std::setprecision(3);
        os << "MyPoint(" << x << ", " << y << ", " << z << ")";
        return os.str();
    }


    MyPoint operator-(const MyPoint& other) const {
        return MyPoint(x - other.x, y - other.y, z - other.z);
    }
};

class MyPoint2 {
public:
    float x, y;

    MyPoint2(float x, float y) : x(x), y(y) {}

    float distance(const MyPoint2& other) const {
        return std::sqrt(std::pow(other.x - x, 2) + std::pow(other.y - y, 2));
    }

    friend std::ostream& operator<<(std::ostream& os, const MyPoint2& point) {
        os << std::fixed << std::setprecision(3);
        os << "MyPoint2(" << point.x << ", " << point.y << ")";
        return os;
    }

    std::string toString() const {
        std::ostringstream os;
        os << std::fixed << std::setprecision(3);
        os << "MyPoint2(" << x << ", " << y << ")";
        return os.str();
    }

    MyPoint2 operator-(const MyPoint2& other) const {
        return MyPoint2(x - other.x, y - other.y);
    }
};

class MyTriangle {
public:
    MyPoint p1, p2, p3;

    MyTriangle(const MyPoint& p1, const MyPoint& p2, const MyPoint& p3) : p1(p1), p2(p2), p3(p3) {}

    float perimeter() const {
        return p1.distance(p2) + p2.distance(p3) + p3.distance(p1);
    }

    float area() const {
        MyPoint a = p2 - p1;
        MyPoint b = p3 - p1;
        return a.cross(b).normalize().distance(MyPoint(0, 0, 0)) / 2.0;
    }

    friend std::ostream& operator<<(std::ostream& os, const MyTriangle& triangle) {
        os << std::fixed << std::setprecision(3);
        os << triangle.p1 << " - " << triangle.p2 << " - " << triangle.p3;
        return os;
    }

    std::string toString() const {
        std::ostringstream os;
        os << std::fixed << std::setprecision(3);
        os << "MyTriangle(" << p1.toString() << ", " << p2.toString() << ", " << p3.toString() << ")";
        return os.str();
    }
};

class MyTriangle2 {
public:
    MyPoint2 p1, p2, p3;

    MyTriangle2(const MyPoint2& p1, const MyPoint2& p2, const MyPoint2& p3) : p1(p1), p2(p2), p3(p3) {}

    float perimeter() const {
        return p1.distance(p2) + p2.distance(p3) + p3.distance(p1);
    }

    float area() const {
        return std::abs((p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y)) / 2.0);
    }

    friend std::ostream& operator<<(std::ostream& os, const MyTriangle2& triangle) {
        os << std::fixed << std::setprecision(3);
        os << "MyTriangle2(" << triangle.p1 << ", " << triangle.p2 << ", " << triangle.p3 << ")";
        return os;
    }

    std::string toString() const {
        std::ostringstream os;
        os << std::fixed << std::setprecision(3);
        os << "MyTriangle2(" << p1.toString() << ", " << p2.toString() << ", " << p3.toString() << ")";
        return os.str();
    }
};

class MyMesh {
public:
    std::vector<MyTriangle> triangles;

    void addTriangle(const MyTriangle& triangle) {
        triangles.push_back(triangle);
    }

    friend std::ostream& operator<<(std::ostream& os, const MyMesh& mesh) {
        os << "MyMesh(";
        for (const auto& triangle : mesh.triangles) {
            os << "\n" << triangle;
        }
        os << "\n)";
        return os;
    }

    std::string toString() const {
        std::ostringstream os;
        os << "MyMesh(";
        for (const auto& triangle : triangles) {
            os << "\n" << triangle.toString();
        }
        os << "\n)";
        return os.str();
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

