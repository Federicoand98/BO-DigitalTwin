// roof-builder.cpp: definisce il punto di ingresso dell'applicazione.
//

#define NOMINMAX 1

#include "roof-builder.h"
#include <iostream> 
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include "Readers/ReaderLas.h"
#include "Grid.h"
#include "Building.h"
#include "Dbscan.h"
#include "UtilsCV.h"
#include "Readers/ReaderCsv.h"

#include "Printer.h"
#include <opencv2/opencv.hpp>


std::vector<std::vector<float>> compute_local_max(const std::vector<std::vector<float>>& v, int kernel_size) {
	int k = kernel_size / 2;
	std::vector<std::vector<float>> max(v.size(), std::vector<float>(v[0].size(), 0.0));
	std::vector<std::vector<float>> max_check(v.size(), std::vector<float>(v[0].size(), 0.0));

	for (int i = k; i < v.size() - k; ++i) {
		for (int j = k; j < v[i].size() - k; ++j) {
			float temp = 0.0;
			for (int m = -k; m <= k; ++m) {
				for (int n = -k; n <= k; ++n) {
					int i_index = i + m;
					int j_index = j + n;

					float val = v[i_index][j_index];
					if (val > temp) {
						temp = val;
					}
				}
			}
			max[i][j] = temp;
		}
	}

	for (int i = 0; i < v.size(); ++i) {
		for (int j = 0; j < v[i].size(); ++j) {
			if (max[i][j] == v[i][j] && v[i][j] != 0.0) {
				max_check[i][j] = 254.0;
			}
			else {
				max_check[i][j] = 1.0;
			}
		}
	}

	return max_check;
}

int main() {
	auto start = std::chrono::high_resolution_clock::now();

	ReaderCsv readerCsv;
	readerCsv.Read(ASSETS_PATH "compactBuildings.csv");

	std::vector<std::string> lines = readerCsv.Ottieni();

	std::vector<std::shared_ptr<Building>> buildings;
	//uint16_t select = 52578;
	uint16_t select = 24069;
	int targetInd;
	std::vector<MyPoint> targetPoints;
	int i = 0;

	for (std::string line : lines) {
		std::shared_ptr<Building> building = BuildingFactory::CreateBuilding(line);
		if (building->GetCodiceOggetto() == select)
			targetInd = i;
		buildings.push_back(building);
		i++;
	}

	readerCsv.Flush();
	std::shared_ptr<Building> targetBuild = buildings.at(targetInd);

	int targetCornerNumb = targetBuild->GetPolygon()->getNumPoints() - 1;
	std::cout << "Corners number: " << targetCornerNumb << std::endl;


	auto geomFactory = geos::geom::GeometryFactory::create();
	
	for (const auto& file : std::filesystem::directory_iterator(ASSETS_PATH "las/")) {
		try {
			ReaderLas readerLas(file.path().string());
			readerLas.Read();

			std::vector<MyPoint>* points = readerLas.Get();
			if (!points->empty()) {
				for (auto &p : *points) {
					if (p.z >= targetBuild->GetQuotaGronda() && p.z <= (targetBuild->GetQuotaGronda() + targetBuild->GetTolleranza())) {
						auto point = geomFactory->createPoint(geos::geom::Coordinate(p.x, p.y, p.z));
						if (targetBuild->GetPolygon()->contains(point.get())) {
							targetPoints.push_back(p);
						}
					}
				}
			}
			points->clear();

			readerLas.Flush();

			
			
		} catch (const pdal::pdal_error &e) {
			std::cerr << "PDAL read error: " << e.what() << std::endl;
			return 1;
		} catch (const std::exception &e) {
			std::cerr << "Exception: " << e.what() << std::endl;
		} catch (...) {
			std::cerr << "An unknown error occurred." << std::endl;
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> diff = end - start;

    std::cout << "Tempo trascorso: " << diff.count() << std::endl;

	std::vector<MyPoint> mainCluster = Dbscan::GetMainCluster(std::span(targetPoints), 0.8, 10);

	auto grid_s = std::chrono::high_resolution_clock::now();

	Grid grid;
	grid.Init(mainCluster, 0.1, 2.0, 0.2);
	grid.FillHoles(3, 3);

	auto grid_e = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff_g = grid_e - grid_s;

	std::cout << "Tempo grid: " << diff_g.count() << std::endl;

	std::vector<std::vector<float>> height_mat = grid.GetHeightMat();

	std::vector<std::vector<float>> f1 = height_mat;

	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::min();

	for (const auto& row : height_mat) {
		for (const auto& z : row) {
			if (z > 0.0f) {
				minZ = std::min(minZ, z);
			}
			maxZ = std::max(maxZ, z);
		}
	}

	for (int i = 0; i < f1.size(); i++) {
		for (int j = 0; j < f1[0].size(); j++) {
			if (f1[i][j] > minZ) {
				f1[i][j] = minZ;
			}
		}
	}

	/*
	std::vector<std::vector<float>> f2 = height_mat;

	for (int i = 0; i < f2.size(); i++) {
		for (int j = 0; j < f2[0].size(); j++) {
			if (f2[i][j] > minZ) {
				f2[i][j] = minZ;
			}
		}
	}
	*/

	//UtilsCV::Show(f1, ColoringMethod::HEIGHT_TO_GRAYSCALE, 1.2);


	cv::Mat image = UtilsCV::GetImage(f1, ColoringMethod::HEIGHT_TO_GRAYSCALE);

	//UtilsCV::Show(image, 1.2);

	cv::Mat blur;
	double sigma = 3.2;
	cv::GaussianBlur(image, blur, cv::Size(0,0), sigma);

	//UtilsCV::Show(blur, 1.2);

	cv::Mat edges;
	cv::Canny(blur, edges, 50, 150);

	//UtilsCV::Show(edges, 1.2);
	
	cv::Mat k5 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
	cv::Mat edges_cl;
	cv::morphologyEx(edges, edges_cl, cv::MORPH_CLOSE, k5, cv::Point(-1,-1), 2);

	cv::Mat cornersTarget = edges_cl;
	std::vector<cv::Point2f> corners;
	int maxCorners = floor(targetCornerNumb * 1.1);
	cv::goodFeaturesToTrack(cornersTarget, corners, maxCorners, 0.1, 10.0, cv::Mat(), 9, false, 0.04);

	cv::Mat blend = cv::Mat::zeros(cornersTarget.size(), CV_MAKETYPE(CV_8U,3));

	for (int i = 0; i < cornersTarget.rows; i++) {
        for (int j = 0; j < cornersTarget.cols; j++) {
            if (cornersTarget.at<uchar>(i, j) > 0) {
                blend.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255); // BGR
            }
        }
    }
	for (size_t i = 0; i < corners.size(); i++) {
        circle(blend, corners[i], 0.1, cv::Scalar(0, 255, 0), 2); // BGR
    }

	std::cout << "Points: " << std::endl;
	for (size_t i = 0; i < corners.size(); i++) {
		MyPoint p = grid.GetGridPointCoord(corners[i].x, corners[i].y);
		std::cout << p << std::endl;
	}

	//UtilsCV::Show(blend, 1.2);
	std::vector<std::vector<float>> lm = compute_local_max(height_mat, 11);
	
	int count = 0;

	for (int i = 0; i < lm.size(); ++i) {
		for (int j = 0; j < lm[i].size(); ++j) {
			if (lm[i][j] == 254.0) {
				count++;
			}
		}
	}

	std::cout << "Max points: " << count << std::endl;

	cv::Mat lmIm = UtilsCV::GetImage(lm, ColoringMethod::HEIGHT_TO_GRAYSCALE);
	UtilsCV::Show(lmIm, 1.2);

	cv::Mat ke = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 2));
	cv::morphologyEx(lmIm, lmIm, cv::MORPH_ERODE, ke, cv::Point(-1, -1), 1);
	UtilsCV::Show(lmIm, 1.2);

	std::vector<cv::Point2f> topPoints;
	int maxTop = floor(targetCornerNumb * 0.9);
	cv::goodFeaturesToTrack(lmIm, topPoints, maxTop, 0.1, 3.0, cv::Mat(), 50, false, 0.04);

	cv::Mat lmBlend = cv::Mat::zeros(lmIm.size(), CV_MAKETYPE(CV_8U, 3));

	for (int i = 0; i < lmIm.rows; i++) {
		for (int j = 0; j < lmIm.cols; j++) {
			if (lmIm.at<uchar>(i, j) > 0) {
				lmBlend.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255); // BGR
			}
		}
	}
	for (size_t i = 0; i < topPoints.size(); i++) {
		circle(lmBlend, topPoints[i], 0.1, cv::Scalar(0, 255, 0), 2); // BGR
	}

	/*
	std::cout << "Points: " << std::endl;
	for (size_t i = 0; i < corners.size(); i++) {
		MyPoint p = grid.GetGridPointCoord(corners[i].x, corners[i].y);
		std::cout << p << std::endl;
	}
	*/

	UtilsCV::Show(lmBlend, 1.2);

	return 0;
}



