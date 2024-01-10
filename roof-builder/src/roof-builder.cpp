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

#include "Printer.h"
#include <opencv2/opencv.hpp>

int main() {

	auto start = std::chrono::high_resolution_clock::now();

	std::ifstream file(ASSETS_PATH "compactBuildings.csv");
	std::string line;
	std::vector<std::string> lines;

	std::vector<std::shared_ptr<Building>> buildings;

	while (std::getline(file, line)) {
		if (line.find("CODICE_OGGETTO") != std::string::npos)
			continue;

		lines.push_back(line);
	}

	//uint16_t select = 52578;
	uint16_t select = 24069;
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

	std::vector<std::vector<size_t>> clusters = dbscan(std::span(targetPoints), 1.0, 10);

	size_t largest_cluster_idx = 0;
	size_t max_size = 0;

	for (size_t i = 0; i < clusters.size(); ++i) {
		if (clusters[i].size() > max_size) {
			max_size = clusters[i].size();
			largest_cluster_idx = i;
		}
	}

	std::vector<MyPoint> largest_cluster;
	for (auto idx : clusters[largest_cluster_idx]) {
		largest_cluster.push_back(targetPoints[idx]);
	}

	Printer::printPoints(largest_cluster, 2.0, 15);

	/*
	std::vector<std::vector<MyPoint>> clusters = Dbscan::FindClusters(targetPoints, 1.0, 10);

	Dbscan::MergeClusters(clusters, 0.5);
	
	Printer::printPoints(clusters[0], 2.0, 20);

	*/


	/*
	
	cv::Mat image = cv::imread(ASSETS_PATH "hg_52578.png", cv::IMREAD_GRAYSCALE);

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
	*/

	return 0;
}
