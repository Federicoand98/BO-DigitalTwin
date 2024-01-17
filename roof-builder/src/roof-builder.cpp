// roof-builder.cpp: definisce il punto di ingresso dell'applicazione.
//

#define NOMINMAX 1
#define VIEW_SCALE 1.5

#include "roof-builder.h"
#include <iostream> 
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <locale>
#include "Readers/ReaderLas.h"
#include "Grid.h"
#include "Building.h"
#include "Dbscan.h"
#include "ImageProcessing/UtilsCV.h"
#include "Readers/ReaderCsv.h"

#include "Printer.h"
#include <opencv2/opencv.hpp>


int main() {
	std::setlocale(LC_ALL, "C");

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

	//UtilsCV::Show(f1, ColoringMethod::HEIGHT_TO_GRAYSCALE, VIEW_SCALE);


	cv::Mat image = UtilsCV::GetImage(f1, ColoringMethod::HEIGHT_TO_GRAYSCALE);

	//UtilsCV::Show(image, VIEW_SCALE);

	cv::Mat blur;
	double sigma = 3.2;
	cv::GaussianBlur(image, blur, cv::Size(0,0), sigma);

	//UtilsCV::Show(blur, VIEW_SCALE);

	cv::Mat edges;
	cv::Canny(blur, edges, 50, 150);

	//UtilsCV::Show(edges, VIEW_SCALE);
	
	cv::Mat k5 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
	cv::Mat edges_cl;
	cv::morphologyEx(edges, edges_cl, cv::MORPH_CLOSE, k5, cv::Point(-1,-1), 2);

	cv::Mat cornersTarget = edges_cl;
	std::vector<cv::Point2f> corners;
	int maxCorners = floor(targetCornerNumb * 3.0);
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

	//UtilsCV::Show(blend, VIEW_SCALE);

	std::vector<std::vector<float>> lm = grid.GetLocalMax(11);

	cv::Mat lmIm = UtilsCV::GetImage(lm, ColoringMethod::HEIGHT_TO_GRAYSCALE);
	//UtilsCV::Show(lmIm, VIEW_SCALE);

	cv::Mat k3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::dilate(lmIm, lmIm, k3);
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(lmIm, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	cv::Mat centers = cv::Mat::zeros(lmIm.size(), CV_8UC1);

	int e = 0;
	for (const auto& contour : contours) {
		cv::Moments m = cv::moments(contour);
		if (m.m00 != 0) {
			int cx = int(m.m10 / m.m00);
			int cy = int(m.m01 / m.m00);

			centers.at<uchar>(cy, cx) = 255;
		}
	}

	std::vector<cv::Point2f> cornersTop;
	for (int i = 0; i < centers.rows; ++i) {
		for (int j = 0; j < centers.cols; ++j) {
			if (centers.at<uchar>(i, j) == 255.0) {
				cornersTop.push_back(cv::Point2f(j, i));
			}
		}
	}

	//UtilsCV::Show(centers, VIEW_SCALE);

	cv::Mat lmBlend = cv::Mat::zeros(lmIm.size(), CV_MAKETYPE(CV_8U, 3));

	for (int i = 0; i < cornersTarget.rows; i++) {
		for (int j = 0; j < cornersTarget.cols; j++) {
			if (cornersTarget.at<uchar>(i, j) > 0) {
				lmBlend.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255); // BGR
			}
		}
	}
	for (size_t i = 0; i < corners.size(); i++) {
		circle(lmBlend, corners[i], 0.1, cv::Scalar(0, 255, 0), 2); // BGR
	}
	for (size_t i = 0; i < cornersTop.size(); i++) {
		circle(lmBlend, cornersTop[i], 0.1, cv::Scalar(255, 0, 0), 2); // BGR
	}

	/*
	std::cout << "Points Bottom: " << std::endl;
	for (size_t i = 0; i < corners.size(); i++) {
		MyPoint p = grid.GetGridPointCoord(corners[i].x, corners[i].y);
		std::cout << p << std::endl;
	}
	std::cout << "Points Top: " << std::endl;
	for (size_t i = 0; i < cornersTop.size(); i++) {
		MyPoint p = grid.GetGridPointCoord(cornersTop[i].x, cornersTop[i].y);
		std::cout << p << std::endl;
	}
	*/

	//UtilsCV::Show(lmBlend, VIEW_SCALE);

	cv::Rect rect(0, 0, lmBlend.cols, lmBlend.rows);
	cv::Subdiv2D subdiv(rect);

	for (size_t i = 0; i < corners.size(); i++) {
		subdiv.insert(corners[i]);
	}
	for (size_t i = 0; i < cornersTop.size(); i++) {
		subdiv.insert(cornersTop[i]);
	}

	std::vector<cv::Vec6f> triangleList;
	subdiv.getTriangleList(triangleList);

	cv::Scalar delaunay_color(128, 0, 128);

	std::ofstream stlFile(OUTPUT_PATH "temp.stl");

	stlFile.imbue(std::locale::classic());
	stlFile << std::fixed << std::setprecision(3);
	stlFile << "solid\n";

	for (size_t i = 0; i < triangleList.size(); ++i) {
		cv::Vec6f t = triangleList[i];
		cv::Point pt0(cvRound(t[0]), cvRound(t[1]));
		cv::Point pt1(cvRound(t[2]), cvRound(t[3]));
		cv::Point pt2(cvRound(t[4]), cvRound(t[5]));

		float c_x = (pt0.x + pt1.x + pt2.x) / 3.0;
		float c_y = (pt0.y + pt1.y + pt2.y) / 3.0;

		if (image.at<uchar>(c_y, c_x) != 255) {
			cv::line(lmBlend, pt0, pt1, delaunay_color, 1);
			cv::line(lmBlend, pt1, pt2, delaunay_color, 1);
			cv::line(lmBlend, pt2, pt0, delaunay_color, 1);

			MyPoint p0 = grid.GetGridPointCoord(pt0.x, pt0.y);
			MyPoint p1 = grid.GetGridPointCoord(pt1.x, pt1.y);
			MyPoint p2 = grid.GetGridPointCoord(pt2.x, pt2.y);

			/*
			MyPoint U = p1 - p0;
			MyPoint V = p2 - p0;

			MyPoint N = U.cross(V).normalize();

			stlFile << "facet normal " << N.x << " " << N.y << " " << N.z << "\n";
			*/
			stlFile << "outer loop\n";
			stlFile << "vertex " << p0.x << " " << p0.y << " " << p0.z << "\n";
			stlFile << "vertex " << p1.x << " " << p1.y << " " << p1.z << "\n";
			stlFile << "vertex " << p2.x << " " << p2.y << " " << p2.z << "\n";
			stlFile << "endloop\n";
			//stlFile << "endf\n";
		}
	}
	stlFile << "endsolid\n";
	stlFile.close();

	UtilsCV::Show(lmBlend, VIEW_SCALE);

	
	return 0;
}
