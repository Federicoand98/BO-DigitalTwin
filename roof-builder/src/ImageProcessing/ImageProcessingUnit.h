#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

class ImageProcessingUnit {
public:
	virtual void Process(cv::Mat& ioMatrix) = 0;
	virtual std::vector<cv::Point2f> Finalize(cv::Mat& ioMatrix, int maxFeatures = 0) = 0;
	virtual bool IsFinalUnit() = 0;
};

class BlurUnit : public ImageProcessingUnit {
public:
	BlurUnit(float sigma = 1.0, cv::Size ksize = cv::Size(0, 0)) : m_IsFinalUnit(false), m_Sigma(sigma), m_Ksize(ksize) {}
	~BlurUnit() {}

	void Process(cv::Mat& ioMatrix) override {
		cv::GaussianBlur(ioMatrix, ioMatrix, m_Ksize, m_Sigma);
	}

	std::vector<cv::Point2f> Finalize(cv::Mat& ioMatrix, int maxFeatures = 0) override {
		return std::vector<cv::Point2f>();
	}

	bool IsFinalUnit() override { return m_IsFinalUnit; }

private:
	bool m_IsFinalUnit;
	float m_Sigma;
	cv::Size m_Ksize;
};

class CannyUnit : public ImageProcessingUnit {
public:
	CannyUnit(float hr = 150.0, float hl = 50.0) : m_IsFinalUnit(false), m_Hr(hr), m_Hl(hl) {}
	~CannyUnit() {}

	void Process(cv::Mat& ioMatrix) override {
		cv::Canny(ioMatrix, ioMatrix, m_Hl, m_Hr);
	}

	std::vector<cv::Point2f> Finalize(cv::Mat& ioMatrix, int maxFeatures = 0) override {
		return std::vector<cv::Point2f>();
	}

	bool IsFinalUnit() override { return m_IsFinalUnit; }

private:
	bool m_IsFinalUnit;
	float m_Hr, m_Hl;
};

class MorphologyUnit : public ImageProcessingUnit {
public:
	MorphologyUnit(cv::MorphTypes op, cv::InputArray kernel, int iters = 1) : m_IsFinalUnit(false), m_Operator(op),
		m_Kernel(kernel), m_Iters(iters) {}
	~MorphologyUnit() {}

	void Process(cv::Mat& ioMatrix) override {
		cv::morphologyEx(ioMatrix, ioMatrix, m_Operator, m_Kernel, cv::Point(-1, -1), m_Iters);
	}

	std::vector<cv::Point2f> Finalize(cv::Mat& inputMatrix, int maxFeatures = 0) override {
		return std::vector<cv::Point2f>();
	}

	bool IsFinalUnit() override { return m_IsFinalUnit; }

private:
	bool m_IsFinalUnit;
	cv::MorphTypes m_Operator;
	cv::InputArray m_Kernel;
	int m_Iters;
};

class FeaturesUnit : public ImageProcessingUnit {
public:
	FeaturesUnit(double qualityLevel, double minDistance, int blockSize) : m_IsFinalUnit(true), m_QualityLevel(qualityLevel),
		m_MinDistance(minDistance), m_BlockSize(blockSize) {}
	~FeaturesUnit() {}

	void Process(cv::Mat& ioMatrix) override {};

	std::vector<cv::Point2f> Finalize(cv::Mat& ioMatrix, int maxFeatures = 0) override {
		std::vector<cv::Point2f> result;
		cv::goodFeaturesToTrack(ioMatrix, result, maxFeatures, m_QualityLevel, m_MinDistance, cv::Mat(), m_BlockSize, false, 0.04);

		return result;
	}

	bool IsFinalUnit() override { return m_IsFinalUnit; }

private:
	bool m_IsFinalUnit;
	double m_QualityLevel, m_MinDistance;
	int m_BlockSize;
};

class FindCentersUnit : public ImageProcessingUnit {
public:
	FindCentersUnit() : m_IsFinalUnit(true) {}
	~FindCentersUnit() {}

	void Process(cv::Mat& ioMatrix) override {};

	std::vector<cv::Point2f> Finalize(cv::Mat& ioMatrix, int maxFeatures = 0) override {
		std::vector<cv::Point2f> result;

		cv::Mat k3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
		cv::dilate(ioMatrix, ioMatrix, k3);
		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(ioMatrix, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		cv::Mat centers = cv::Mat::zeros(ioMatrix.size(), CV_8UC1);

		for (const auto& contour : contours) {
			cv::Moments m = cv::moments(contour);
			if (m.m00 != 0) {
				int cx = int(m.m10 / m.m00);
				int cy = int(m.m01 / m.m00);

				centers.at<uchar>(cy, cx) = 255;
			}
		}

		for (int i = 0; i < centers.rows; ++i) {
			for (int j = 0; j < centers.cols; ++j) {
				if (centers.at<uchar>(i, j) == 255.0) {
					result.push_back(cv::Point2f(j, i));
				}
			}
		}

		return result;
	}

	bool IsFinalUnit() override { return m_IsFinalUnit; }

private:
	bool m_IsFinalUnit;
};