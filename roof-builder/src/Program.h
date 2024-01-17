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

class Program {
public:
	Program();
	~Program();

	static Program& Get();

	void Execute();
};

static Program* s_Instance = nullptr;

Program::Program() {}

Program::~Program() {}

Program& Program::Get() {
	return s_Instance;
}

void Program::Execute() {
	std::setlocale(LC_ALL, "C");

	//uint16_t select = 52578;
	uint16_t select = 24069;

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
	std::shared_ptr<Building> targetBuild = buildings.at(targetInd);

	// TODO: possibile filtro sugli edifici che appartengono a determinati tile
	for (std::shared_ptr<Building> building : buildings) {

		int targetCornerNumb = targetBuild->GetPolygon()->getNumPoints() - 1;
		std::cout << "Corners number: " << targetCornerNumb << std::endl;

		auto geomFactory = geos::geom::GeometryFactory::create();
		
		// TODO: iterare su tutti i tile del building
		for (const auto& file : std::filesystem::directory_iterator(ASSETS_PATH "las/")) {
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
		}

		std::vector<MyPoint> mainCluster = Dbscan::GetMainCluster(std::span(targetPoints), 0.8, 10);

		Grid grid;
		grid.Init(mainCluster, 0.1, 2.0, 0.2);
		grid.FillHoles(3, 3);
		targetPoints.clear();

		std::vector<std::vector<float>> br = grid.GetBooleanRoof();
		std::vector<std::vector<float>> lm = grid.GetLocalMax();

		std::shared_ptr<ImageProcesser> roofEdgeProcesser = ImageProcesserFactory::CreateEdgePipeline(br);
		std::shared_ptr<ImageProcesser> ridgeEdgeProcesser = ImageProcesserFactory::CreateRidgePipeline(lm);

		roofEdgeProcesser->Process();
		std::vector<cv::Point2f> roofResult = roofEdgeProcesser->GetOutput();	// TODO: cambiare cv::Point2f

		ridgeEdgeProcesser->Process();
		std::vector<cv::Point2f> ridgeResult = ridgeEdgeProcesser->GetOutput();	// TODO: cambiare cv::Point2f


	}
}
