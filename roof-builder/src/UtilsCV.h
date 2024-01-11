#pragma once

#include <vector>
#include <opencv2/opencv.hpp>


class UtilsCV {
public:
    static cv::Mat GetImage(std::vector<std::vector<float>>& mat) {
        int rows = mat.size();
        int cols = mat[0].size();

        cv::Mat image(rows, cols, CV_32F);

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                image.at<float>(i, j) = mat[i][j];
            }
        }

        cv::normalize(image, image, 0, 255, cv::NORM_MINMAX, CV_8U);
        cv::flip(image, image, 0);

        return image;
    }

    static void Show(cv::Mat& image, float scale) {
        cv::Mat res;
        cv::resize(image, res, cv::Size(), scale, scale, cv::INTER_LINEAR);
        cv::imshow("Result", res);

        cv::waitKey(0);
    }
};