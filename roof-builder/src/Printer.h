#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include "MyPoint.h"

class Printer {
public:
    static void printPoints(const std::vector<MyPoint>& targetPoints, float tol, int scale) {
        float min_x = std::min_element(targetPoints.begin(), targetPoints.end(), [](MyPoint const a, MyPoint const b) { return a.x < b.x; })->x - tol;
        float max_x = std::max_element(targetPoints.begin(), targetPoints.end(), [](MyPoint const a, MyPoint const b) { return a.x < b.x; })->x + tol;
        float min_y = std::min_element(targetPoints.begin(), targetPoints.end(), [](MyPoint const a, MyPoint const b) { return a.y < b.y; })->y - tol;
        float max_y = std::max_element(targetPoints.begin(), targetPoints.end(), [](MyPoint const a, MyPoint const b) { return a.y < b.y; })->y + tol;

        int w = (int)(max_x - min_x) * scale;
        int h = (int)(max_y - min_y) * scale;

        cv::Mat image = cv::Mat::zeros(h, w, CV_8UC3);

        for (const auto& point : targetPoints) {
            int x = ((point.x - min_x) / (max_x - min_x)) * w;
            int y = h - ((point.y - min_y) / (max_y - min_y)) * h;

            cv::circle(image, cv::Point(x, y), 0.1, cv::Scalar(0, 255, 0), -1);
        }

        cv::imshow("Image", image);
        cv::waitKey(0);
    }
};
