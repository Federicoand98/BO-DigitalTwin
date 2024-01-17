#pragma once

#include <vector>
#include <math.h>
#include <opencv2/opencv.hpp>
#include "ImageProcessingUnit.h"

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
        color.r = 1.0f;
        color.g = 1.0f;
        color.b = 1.0f;
    }
    else {
        float normalizedValue = (z - min) / (max - min);

        float hue = normalizedValue * 360.0f;
        float saturation = 1.0f;
        float value = 1.0f;

        int h = static_cast<int>(hue / 60.0f) % 6;
        float f = hue / 60.0f - h;
        float p = value * (1.0f - saturation);
        float q = value * (1.0f - f * saturation);
        float t = value * (1.0f - (1.0f - f) * saturation);

        switch (h) {
        case 0:
            color.r = static_cast<int>(value * 255);
            color.g = static_cast<int>(t * 255);
            color.b = static_cast<int>(p * 255);
            break;
        case 1:
            color.r = static_cast<int>(q * 255);
            color.g = static_cast<int>(value * 255);
            color.b = static_cast<int>(p * 255);
            break;
        case 2:
            color.r = static_cast<int>(p * 255);
            color.g = static_cast<int>(value * 255);
            color.b = static_cast<int>(t * 255);
            break;
        case 3:
            color.r = static_cast<int>(p * 255);
            color.g = static_cast<int>(q * 255);
            color.b = static_cast<int>(value * 255);
            break;
        case 4:
            color.r = static_cast<int>(t * 255);
            color.g = static_cast<int>(p * 255);
            color.b = static_cast<int>(value * 255);
            break;
        case 5:
        default:
            color.r = static_cast<int>(value * 255);
            color.g = static_cast<int>(p * 255);
            color.b = static_cast<int>(q * 255);
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

    static void Show(cv::Mat& image, float scale) {
        cv::Mat res;
        cv::resize(image, res, cv::Size(), scale, scale, cv::INTER_LINEAR);
        cv::imshow("Result", res);

        cv::waitKey(0);
        cv::destroyAllWindows();
        cv::waitKey(1);
    }

    static void Show(std::vector<std::vector<float>>& mat, ColoringMethod method, float scale) {
        cv::Mat image = GetImage(mat, method);

        cv::Mat res;
        cv::resize(image, res, cv::Size(), scale, scale, cv::INTER_LINEAR);
        cv::imshow("Result", res);

        cv::waitKey(0);
        cv::destroyAllWindows();
        cv::waitKey(1);
    }
};

class ImageProcesser {
public:
    ImageProcesser(std::vector<std::vector<float>>& baseMatrix) {
        m_Image = UtilsCV::GetImage(baseMatrix, ColoringMethod::HEIGHT_TO_GRAYSCALE);
    }

    ~ImageProcesser() {
        for (ImageProcessingUnit* unit : m_ProccessingUnits) {
            delete unit;
        }

        m_ProccessingUnits.clear();
    }

    void AddProcessingUnit(ImageProcessingUnit* processingUnit) {
        m_ProccessingUnits.push_back(processingUnit);
    }

    void Process() {
        for (ImageProcessingUnit* unit : m_ProccessingUnits) {
            if (unit->IsFinalUnit())
                unit->Finalize(m_Image);
            else
                unit->Process(m_Image);
        }
    }

private:
    std::vector<ImageProcessingUnit*> m_ProccessingUnits;
    cv::Mat m_Image;
};

class ImageProcesserFactory {
public:
    static std::shared_ptr<ImageProcesser> CreateEdgePipeline(std::vector<std::vector<float>>& matrix) {
        std::shared_ptr<ImageProcesser> processer = std::make_shared<ImageProcesser>(matrix);

        ImageProcessingUnit* blur = new BlurUnit(3.5);
        ImageProcessingUnit* canny = new CannyUnit(50.0, 150.0);
        ImageProcessingUnit* morph = new MorphologyUnit(cv::MORPH_CLOSE, 
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1,-1), 2);
        ImageProcessingUnit* feats = new FeaturesUnit(0.1, 10.0, 9);
        processer->AddProcessingUnit(blur);
        processer->AddProcessingUnit(canny);
        processer->AddProcessingUnit(morph);
        processer->AddProcessingUnit(feats);

        return processer;
    }

    static std::shared_ptr<ImageProcesser> CreateRidgePipeline(std::vector<std::vector<float>>& matrix) {
        std::shared_ptr<ImageProcesser> processer = std::make_shared<ImageProcesser>(matrix);

        ImageProcessingUnit* centers = new FindCentersUnit();
        processer->AddProcessingUnit(centers);

        return processer;
    }
};

