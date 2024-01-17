#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

class ImageProcessingUnit {
public:
	virtual void Process(cv::Mat& inputMatrix);
	virtual std::vector<cv::Point2f> Finalize(cv::Mat& inputMatrix, int maxFeatures = 0);
	virtual bool IsFinalUnit();
};

class CannyUnit : public ImageProcessingUnit {
public:
	CannyUnit(float sigma = 1.0, float hr = 150.0, float hl = 50.0) : m_IsFinalUnit(false), m_Sigma(sigma), m_Hr(hr), m_Hl(hl) {}
	~CannyUnit() {}

	void Process(cv::Mat& inputMatrix) override {
		cv::GaussianBlur(inputMatrix, inputMatrix, cv::Size(0,0), m_Sigma);
		cv::Canny(inputMatrix, inputMatrix, m_Hl, m_Hr);
	}

private:
	bool m_IsFinalUnit;
	float m_Sigma, m_Hr, m_Hl;
};