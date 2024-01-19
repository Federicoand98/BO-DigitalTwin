#pragma once

#include <iostream> 
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <locale>
#include <algorithm>
#include "Readers/ReaderLas.h"
#include "Grid.h"
#include "Building.h"
#include "Dbscan.h"
#include "ImageProcessing/UtilsCV.h"
#include "Readers/ReaderCsv.h"
#include "Printer.h"
#include "Triangle/TriangleWrap.h"
#include "Exporter.h"

#define SHOW_RESULT true
#define SHOW_STEPS false

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
	std::vector<MyMesh> meshes;
	std::string filePath(OUTPUT_PATH "temp.stl");

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

		std::shared_ptr<ImageProcesser> roofEdgeProcesser = ImageProcesserFactory::CreateEdgePipeline(br, SHOW_STEPS);
		std::shared_ptr<ImageProcesser> ridgeEdgeProcesser = ImageProcesserFactory::CreateRidgePipeline(lm, SHOW_STEPS);

		float safetyFactor = 1.2;
		roofEdgeProcesser->Process(buildingCornerNumb*safetyFactor);
		std::vector<MyPoint2> roofResult = roofEdgeProcesser->GetOutput();

		ridgeEdgeProcesser->Process();
		std::vector<MyPoint2> ridgeResult = ridgeEdgeProcesser->GetOutput();

		TriangleWrapper triWrap;
		triWrap.Initialize();
		triWrap.UploadPoints(roofResult, ridgeResult);
		std::vector<MyTriangle2> triangles2 = triWrap.Triangulate();

		std::vector<MyTriangle> triangles;
		for (MyTriangle2 tri2 : triangles2) {
			MyPoint p1 = grid.GetGridPointCoord(tri2.p1.x, tri2.p1.y);
			MyPoint p2 = grid.GetGridPointCoord(tri2.p2.x, tri2.p2.y);
			MyPoint p3 = grid.GetGridPointCoord(tri2.p3.x, tri2.p3.y);

			triangles.push_back({ p1, p2, p3 });
		}

	
		meshes.push_back(MyMesh(triangles));
		


		if (SHOW_RESULT) {
			cv::Mat resImage = cv::Mat::zeros(cv::Size(br.size(), br[0].size()), CV_MAKETYPE(CV_8U, 3));
			for (size_t i = 0; i < roofResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(roofResult[i].x)), static_cast<int>(std::round(roofResult[i].y)));
				circle(resImage, temp, 0.1, cv::Scalar(0, 255, 0), 2); // BGR
			}
			for (size_t i = 0; i < ridgeResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(ridgeResult[i].x)), static_cast<int>(std::round(ridgeResult[i].y)));
				circle(resImage, temp, 0.1, cv::Scalar(255, 0, 0), 2); // BGR
			}
			cv::Scalar delaunay_color(128, 0, 128);

			for (const auto& triangle : triangles2) {
				cv::Point pt1(triangle.p1.x, triangle.p1.y);
				cv::Point pt2(triangle.p2.x, triangle.p2.y);
				cv::Point pt3(triangle.p3.x, triangle.p3.y);

				cv::line(resImage, pt1, pt2, delaunay_color, 1);
				cv::line(resImage, pt2, pt3, delaunay_color, 1);
				cv::line(resImage, pt3, pt1, delaunay_color, 1);
			}
			UtilsCV::Show(resImage);
		}
	}

	Exporter::ExportStl(meshes, filePath);

}
