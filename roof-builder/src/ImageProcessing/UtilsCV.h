#pragma once

#define VIEW_SCALE 2.0
#define SAVE true

#include <vector>
#include <math.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include "ImageProcessingUnit.h"
#include "../MyPoint.h"

enum class ColoringMethod {
    HEIGHT_TO_GRAYSCALE,
    HEIGHT_TO_COLOR
};

struct RGBColor {
    float r;
    float g;
    float b;
};

static RGBColor HeightToRainbow(float z, float min, float max) {
    RGBColor color;

    if (z == 0.0f) {
        color.r = 255.0f;
        color.g = 255.0f;
        color.b = 255.0f;
    }
    else {
        float normalizedValue = (z - min) / (max - min);

        // Adatta il range di hue per evitare la ripetizione del rosso agli estremi
        float hue = normalizedValue * 310.0f; // Parte da 0 gradi e va fino a 310 gradi
        float saturation = 1.0f;
        float value = 1.0f;

        int h = static_cast<int>(hue / 60.0f) % 6;
        float f = hue / 60.0f - h;
        float p = value * (1.0f - saturation);
        float q = value * (1.0f - f * saturation);
        float t = value * (1.0f - (1.0f - f) * saturation);

        switch (h) {
        case 0:
            color.r = value * 255;
            color.g = t * 255;
            color.b = p * 255;
            break;
        case 1:
            color.r = q * 255;
            color.g = value * 255;
            color.b = p * 255;
            break;
        case 2:
            color.r = p * 255;
            color.g = value * 255;
            color.b = t * 255;
            break;
        case 3:
            color.r = p * 255;
            color.g = q * 255;
            color.b = value * 255;
            break;
        case 4:
            color.r = t * 255;
            color.g = p * 255;
            color.b = value * 255;
            break;
        case 5:
        default:
            color.r = value * 255;
            color.g = p * 255;
            color.b = q * 255;
            break;
        }
    }

    return color;
}

static uchar HeightToGrayscale(float z, float minZ, float maxZ) {
    if (z == 0.0f) {
        return static_cast<uchar>(255);
    }
    else {
        float normalized_z = (z - minZ) / (maxZ - minZ);
        return static_cast<uchar>(normalized_z * 255.0f);
    }
}

class UtilsCV {
public:
    static cv::Mat GetImage(std::vector<std::vector<float>>& mat, ColoringMethod method) {
        int rows = mat.size();
        int cols = mat[0].size();

        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::min();

        for (const auto& row : mat) {
            for (const auto& z : row) {
                if (z > 0.0f) {
                    minZ = std::min(minZ, z);
                }
                maxZ = std::max(maxZ, z);
            }
        }

        cv::Mat image;

        switch (method) {
        case ColoringMethod::HEIGHT_TO_GRAYSCALE:
            image = cv::Mat(rows, cols, CV_8U);
            break;
        case ColoringMethod::HEIGHT_TO_COLOR:
            image = cv::Mat(rows, cols, CV_8UC3);
            break;
        }

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                switch (method) {
                case ColoringMethod::HEIGHT_TO_GRAYSCALE:
                    image.at<uchar>(i, j) = HeightToGrayscale(mat[i][j], minZ, maxZ);
                    break;
                case ColoringMethod::HEIGHT_TO_COLOR:
                    RGBColor color = HeightToRainbow(mat[i][j], minZ, maxZ);
                    image.at<cv::Vec3b>(i, j) = cv::Vec3b(color.b, color.g, color.r);
                    break;
                }
            }
        }

        cv::rotate(image, image, cv::ROTATE_90_COUNTERCLOCKWISE);

        return image;
    }

    static void ShowPoints(const std::vector<MyPoint>& targetPoints, float tol, int scale, bool filterZeros, std::vector<std::pair<MyPoint, MyPoint>> edges) {
        if (targetPoints.empty()) return;

        float min_x = edges[0].first.x, max_x = edges[0].first.x;
        float min_y = edges[0].first.y, max_y = edges[0].first.y;

        for (const auto& edge : edges) {
            min_x = std::min(min_x, edge.first.x);
            max_x = std::max(max_x, edge.first.x);
            min_y = std::min(min_y, edge.first.y);
            max_y = std::max(max_y, edge.first.y);

            min_x = std::min(min_x, edge.second.x);
            max_x = std::max(max_x, edge.second.x);
            min_y = std::min(min_y, edge.second.y);
            max_y = std::max(max_y, edge.second.y);
        }

        min_x -= tol;
        max_x += tol;
        min_y -= tol;
        max_y += tol;

        float min_z = std::numeric_limits<float>::max();
        float max_z = std::numeric_limits<float>::lowest();

        for (const auto& point : targetPoints) {
            if (point.z > 0) { // Ignora z = 0
                if (point.z < min_z) min_z = point.z;
                if (point.z > max_z) max_z = point.z;
            }
        }

        float range_x = max_x - min_x;
        float range_y = max_y - min_y;
        float max_range = std::max(range_x, range_y);

        int w = (int)(range_x * scale);
        int h = (int)(range_y * scale);

        cv::Mat image = cv::Mat(cv::Size(w, h), CV_8UC3, cv::Scalar(255, 255, 255));

        cv::Scalar edgesColor(12, 12, 12);

        for (int i = 0; i < edges.size(); i++) {
            int x1 = ((edges[i].first.x - min_x) / range_x) * w;
            int y1 = h - ((edges[i].first.y - min_y) / range_y) * h;

            int x2 = ((edges[i].second.x - min_x) / range_x) * w;
            int y2 = h - ((edges[i].second.y - min_y) / range_y) * h;

            cv::line(image, cv::Point(x1, y1), cv::Point(x2, y2), edgesColor, 1);
        }

        for (const auto& point : targetPoints) {
            int x = ((point.x - min_x) / range_x) * w;
            int y = h - ((point.y - min_y) / range_y) * h;

            if (!filterZeros || (filterZeros && point.z > 0)) {
                RGBColor color = HeightToRainbow(point.z, min_z, max_z);
                cv::circle(image, cv::Point(x, y), 1.0, cv::Vec3b(color.b, color.g, color.r), -1);
            }
        }

        Show(image);
    }

    static void Show(cv::Mat& image, float scale = VIEW_SCALE) {
        cv::Mat res;
        cv::resize(image, res, cv::Size(), scale, scale, cv::INTER_CUBIC);

        if (SAVE) {
            if (res.channels() == 1 && res.at<uchar>(0, 0) != 255) {
                res = 255 - res;
            }
            Save("out", res);
        }

        cv::imshow("Result", res);

        cv::waitKey(0);
        cv::destroyAllWindows();
        cv::waitKey(1);
    }

    static void Show(std::vector<std::vector<float>>& mat, ColoringMethod method, float scale = VIEW_SCALE) {
        cv::Mat image = GetImage(mat, method);

        cv::Mat res;
        cv::resize(image, res, cv::Size(), scale, scale, cv::INTER_CUBIC);

        if (SAVE) {
            if (res.channels() == 1 && res.at<uchar>(0, 0) != 255) {
                res = 255 - res;
            }
            Save("out", res);
        }

        cv::imshow("Result", res);

        cv::waitKey(0);
        cv::destroyAllWindows();
        cv::waitKey(1);
    }

    static void Save(std::string name, cv::Mat& image) {
        int i = 1;

        std::string ext = ".png";

        std::string output_filename = OUTPUT_PATH "//img//" + name + "_" + std::to_string(i) + ext;

        while (std::filesystem::exists(output_filename)) {
            i += 1;
            output_filename = OUTPUT_PATH "//img//" + name + "_" + std::to_string(i) + ext;
        }

        // Salva l'immagine con il nuovo nome del file
        cv::imwrite(output_filename, image);
    }
};

class ImageProcesser {
public:
    ImageProcesser(std::vector<std::vector<float>>& baseMatrix, bool showSteps) {
        m_Image = UtilsCV::GetImage(baseMatrix, ColoringMethod::HEIGHT_TO_GRAYSCALE);
        m_ShowSteps = showSteps;
    }

    ~ImageProcesser() {
        for (ImageProcessingUnit* unit : m_ProccessingUnits) {
            delete unit;
        }

        m_ProccessingUnits.clear();
        m_ResultPoint.clear();
    }

    void AddProcessingUnit(ImageProcessingUnit* processingUnit) {
        m_ProccessingUnits.push_back(processingUnit);
    }

    void Process(int maxFeatures = 0) {
        if (m_ShowSteps)
            UtilsCV::Show(m_Image, VIEW_SCALE);
        for (ImageProcessingUnit* unit : m_ProccessingUnits) {
            if (unit->IsFinalUnit()) {
                m_ResultPoint = unit->Finalize(m_Image, maxFeatures);
                if (m_ShowSteps) {
                    cv::Mat tempImg = cv::Mat(m_Image.size(), CV_8UC3, cv::Scalar(255, 255, 255));

                    int maxS = (m_Image.size().height > m_Image.size().width) ? m_Image.size().height : m_Image.size().width;
                    int cs = (maxS / 300) + 2;

                    for (int i = 0; i < m_Image.rows; i++) {
                        for (int j = 0; j < m_Image.cols; j++) {
                            if (m_Image.at<uchar>(i, j) > 0) {
                                tempImg.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 0); // BGR
                            }
                        }
                    }
                    for (size_t i = 0; i < m_ResultPoint.size(); i++) {
                        circle(tempImg, m_ResultPoint[i], cs, cv::Scalar(224, 183, 74), -1); // BGR
                    }

                    UtilsCV::Show(tempImg, VIEW_SCALE);
                }
            }
            else {
                unit->Process(m_Image);
                if (m_ShowSteps)
                    UtilsCV::Show(m_Image, VIEW_SCALE);
            }
        }
    }

    cv::Mat GetImage() { return m_Image; }

    cv::Mat GetCFill() {
        cv::Mat res = cv::Mat::zeros(m_Image.size(), CV_8U);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(m_Image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        double maxArea = 0;
        int maxAreaInd = -1;

        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            if (area > maxArea) {
                maxArea = area;
                maxAreaInd = static_cast<int>(i);
            }
        }

        if (maxAreaInd != -1) {
            cv::drawContours(res, contours, maxAreaInd, 255.0, cv::FILLED);
        }
        return res;
    }

    std::vector<MyPoint2> GetOutput() {
        std::vector<MyPoint2> myPoints;
        for (const auto& point : m_ResultPoint) {
            myPoints.push_back(MyPoint2(point.x, point.y));
        }
        return myPoints;
    }

private:
    std::vector<ImageProcessingUnit*> m_ProccessingUnits;
    std::vector<cv::Point2f> m_ResultPoint;
    cv::Mat m_Image;
    bool m_ShowSteps;
};

class ImageProcesserFactory {
public:
    static std::shared_ptr<ImageProcesser> CreateEdgePipeline(std::vector<std::vector<float>>& matrix, bool showSteps = false) {
        std::shared_ptr<ImageProcesser> processer = std::make_shared<ImageProcesser>(matrix, showSteps);

        ImageProcessingUnit* blur = new BlurUnit(3.5);
        ImageProcessingUnit* canny = new CannyUnit(20.0, 80.0);
        ImageProcessingUnit* morph = new MorphologyUnit(cv::MORPH_CLOSE, 
            cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(5, 5)), 4);
        ImageProcessingUnit* feats = new FeaturesUnit(0.1, 10.0, 9);
        processer->AddProcessingUnit(blur);
        processer->AddProcessingUnit(canny);
        processer->AddProcessingUnit(morph);
        processer->AddProcessingUnit(feats);

        return processer;
    }

    static std::shared_ptr<ImageProcesser> CreateRidgePipeline(std::vector<std::vector<float>>& matrix, bool showSteps = false) {
        std::shared_ptr<ImageProcesser> processer = std::make_shared<ImageProcesser>(matrix, showSteps);

        ImageProcessingUnit* centers = new FindCentersUnit();
        processer->AddProcessingUnit(centers);

        return processer;
    }
};

