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

int main() {
	auto start = std::chrono::high_resolution_clock::now();

	ReaderCsv readerCsv;
	readerCsv.Read(ASSETS_PATH "compactBuildings.csv");

	std::vector<std::string> lines = readerCsv.Ottieni();

	std::vector<std::shared_ptr<Building>> buildings;
	uint16_t select = 52578;
	//uint16_t select = 24069;
	int target_ind;
	std::vector<MyPoint> targetPoints;
	int i = 0;

	for (std::string line : lines) {
		std::shared_ptr<Building> building = BuildingFactory::CreateBuilding(line);
		if (building->GetCodiceOggetto() == select)
			target_ind = i;
		buildings.push_back(building);
		i++;
	}

	readerCsv.Flush();

	auto geomFactory = geos::geom::GeometryFactory::create();
	
	for (const auto& file : std::filesystem::directory_iterator(ASSETS_PATH "las/")) {
		try {
			ReaderLas readerLas(file.path().string());

			readerLas.Read();

			std::vector<MyPoint>* points = readerLas.Get();
			
			if (!points->empty()) {
				for (auto &p : *points) {
					auto point = geomFactory->createPoint(geos::geom::Coordinate(p.x, p.y, 10.0));
					if (buildings.at(target_ind)->GetPolygon()->contains(point.get())) {
						targetPoints.push_back(p);
					}
				}
			}
			points->clear();

			readerLas.Flush();

			std::cout << "\nDone." << std::endl;
			
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

	//Printer::printPoints(mainCluster, 2.0, 15);

	auto grid_s = std::chrono::high_resolution_clock::now();

	Grid grid;
	grid.Init(mainCluster, 0.1, 2.0, 0.2);
	grid.FillHoles(5);

	auto grid_e = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff_g = grid_e - grid_s;

	std::cout << "Tempo grid: " << diff_g.count() << std::endl;

	std::vector<std::vector<float>> height_mat = grid.GetHeightMat();
	std::vector<MyPoint> gridPoints = grid.GetPoints();

	cv::Mat image = UtilsCV::GetImage(height_mat, ColoringMethod::HEIGHT_TO_GRAYSCALE);


	cv::Mat blur;
	double sigma = 3.5;
	cv::GaussianBlur(image, blur, cv::Size(0,0), sigma);

	cv::Mat edges;
	cv::Canny(blur, edges, 50, 80);

	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
	cv::Mat edges_cl;
	cv::morphologyEx(edges, edges_cl, cv::MORPH_CLOSE, kernel);

	std::vector<cv::Point2f> corners;
	cv::goodFeaturesToTrack(edges_cl, corners, 6, 0.1, 10.0, cv::Mat(), 9, false, 0.04);

	cv::Mat blend = cv::Mat::zeros(edges_cl.size(), CV_MAKETYPE(CV_8U,3));

	for (int i = 0; i < edges_cl.rows; i++) {
        for (int j = 0; j < edges_cl.cols; j++) {
            if (edges_cl.at<uchar>(i, j) > 0) {
                blend.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255); // BGR
            }
        }
    }
	for (size_t i = 0; i < corners.size(); i++) {
        circle(blend, corners[i], 0.1, cv::Scalar(0, 255, 0), 2); // BGR
    }

	cv::Mat show;
	cv::resize(blend, show, cv::Size(), 1.5, 1.5, cv::INTER_LINEAR);
	cv::imshow("Result", show);

	cv::waitKey(0);

	return 0;
}
