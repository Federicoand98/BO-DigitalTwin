#pragma once

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

#define SHOW_RESULT true

class Program {
public:
	Program();
	~Program();

	static Program& Get();

	void Execute();
};

static Program* s_Instance = nullptr;

Program::Program() { s_Instance = this; }

Program::~Program() {}

Program& Program::Get() {
	return *s_Instance;
}

void Program::Execute() {
	std::setlocale(LC_ALL, "C");

	uint16_t select = 52578;
	//uint16_t select = 24069;

	ReaderCsv readerCsv;
	readerCsv.Read(ASSETS_PATH "compactBuildings.csv");

	std::vector<std::string> lines = readerCsv.Ottieni();

	std::vector<std::shared_ptr<Building>> buildings;
	std::vector<MyPoint> targetPoints;

	for (std::string line : lines) {
		std::shared_ptr<Building> building = BuildingFactory::CreateBuilding(line);
		if (building->GetCodiceOggetto() == select) {
			buildings.push_back(building);
		}
	}

	readerCsv.Flush();

	// TODO: possibile filtro sugli edifici che appartengono a determinati tile
	for (std::shared_ptr<Building> building : buildings) {

		int buildingCornerNumb = building->GetPolygon()->getNumPoints() - 1;
		std::cout << "Corners number: " << buildingCornerNumb << std::endl;

		auto geomFactory = geos::geom::GeometryFactory::create();
		
		// TODO: iterare su tutti i tile del building
		for (const auto& file : std::filesystem::directory_iterator(ASSETS_PATH "las/")) {
			ReaderLas readerLas(file.path().string());
			readerLas.Read();

			std::vector<MyPoint>* points = readerLas.Get();
			if (!points->empty()) {
				for (auto &p : *points) {
					if (p.z >= building->GetQuotaGronda() && p.z <= (building->GetQuotaGronda() + building->GetTolleranza())) {
						auto point = geomFactory->createPoint(geos::geom::Coordinate(p.x, p.y, p.z));
						if (building->GetPolygon()->contains(point.get())) {
							targetPoints.push_back(p);
						}
					}
				}
			}

			points->clear();
			readerLas.Flush();
		}

		std::vector<MyPoint> mainCluster = Dbscan::GetMainCluster(std::span(targetPoints), 0.8, 10);

		Grid grid;
		grid.Init(mainCluster, 0.1, 2.0, 0.2);
		grid.FillHoles(3, 3);
		targetPoints.clear();


		std::vector<std::vector<float>> br = grid.GetBooleanRoof();
		std::vector<std::vector<float>> lm = grid.GetLocalMax(11);

		std::shared_ptr<ImageProcesser> roofEdgeProcesser = ImageProcesserFactory::CreateEdgePipeline(br);
		std::shared_ptr<ImageProcesser> ridgeEdgeProcesser = ImageProcesserFactory::CreateRidgePipeline(lm);

		roofEdgeProcesser->Process(buildingCornerNumb);
		std::vector<cv::Point2f> roofResult = roofEdgeProcesser->GetOutput();	// TODO: cambiare cv::Point2f

		ridgeEdgeProcesser->Process();
		std::vector<cv::Point2f> ridgeResult = ridgeEdgeProcesser->GetOutput();	// TODO: cambiare cv::Point2f
		

		cv::Mat resImage = cv::Mat::zeros(cv::Size(br.size(), br[0].size()), CV_MAKETYPE(CV_8U, 3));

		cv::Rect rect(0, 0, br.size(), br[0].size());
		cv::Subdiv2D subdiv(rect);

		for (size_t i = 0; i < roofResult.size(); i++) {
			subdiv.insert(roofResult[i]);
			if (SHOW_RESULT)
				circle(resImage, roofResult[i], 0.1, cv::Scalar(0, 255, 0), 2); // BGR
		}
		for (size_t i = 0; i < ridgeResult.size(); i++) {
			subdiv.insert(ridgeResult[i]);
			if (SHOW_RESULT)
				circle(resImage, ridgeResult[i], 0.1, cv::Scalar(255, 0, 0), 2); // BGR
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

			if (br[c_x][br[0].size()-c_y] != 0) {
				if (SHOW_RESULT) {
					cv::line(resImage, pt0, pt1, delaunay_color, 1);
					cv::line(resImage, pt1, pt2, delaunay_color, 1);
					cv::line(resImage, pt2, pt0, delaunay_color, 1);
				}

				MyPoint p0 = grid.GetGridPointCoord(pt0.x, pt0.y);
				MyPoint p1 = grid.GetGridPointCoord(pt1.x, pt1.y);
				MyPoint p2 = grid.GetGridPointCoord(pt2.x, pt2.y);

				stlFile << "outer loop\n";
				stlFile << "vertex " << p0.x << " " << p0.y << " " << p0.z << "\n";
				stlFile << "vertex " << p1.x << " " << p1.y << " " << p1.z << "\n";
				stlFile << "vertex " << p2.x << " " << p2.y << " " << p2.z << "\n";
				stlFile << "endloop\n";
			}
			
		}
		stlFile << "endsolid\n";
		stlFile.close();

		if (SHOW_RESULT)
			UtilsCV::Show(resImage);
	}
}
