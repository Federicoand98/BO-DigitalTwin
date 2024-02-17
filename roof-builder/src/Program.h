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
#include <execution>
#include "Readers/ReaderLas.h"
#include "Grid.h"
#include "Building.h"
#include "Dbscan.h"
#include "ImageProcessing/UtilsCV.h"
#include "Readers/ReaderCsv.h"
#include "Triangle/TriangleWrap.h"
#include "Exporter.h"

#define DEBUG 1
#define PRINT_TEMP false

#define SHOW_RESULT false
#define SHOW_STEPS false
#define SHOW_CLEAN_EDGES false 

#define SELECT_METHOD 1 //0 single building, 1 single las, 2 all las in dir

#define LAS_PATH ASSETS_PATH "las/"

class Program {
public:
	Program();
	~Program();

	static Program& Get();

	void Execute();
private:
	std::map<std::string, std::vector<MyPoint>> m_lasData;
};

static Program* s_Instance = nullptr;

Program::Program() { s_Instance = this; }

Program::~Program() {}

Program& Program::Get() {
	return *s_Instance;
}

void Program::Execute() {
	std::setlocale(LC_ALL, "C");

	std::cout << "### Starting Data Load Process..." << std::endl;

	//uint16_t select = 52578; //l-shape
	uint16_t select = 24069; //top-t

	std::string selectLas = "32_684000_4930000.las";

	//uint16_t select = 47924; //dozza
	//std::string selectLas = "32_685000_4930000.las";

	//uint16_t select = 24921; //coso strano ma bellino
	//std::string selectLas = "32_686000_4928500.las";

	//std::string selectLas = "32_686000_4929500.las";

	std::vector<std::string> lasNames;

	for (const auto& entry : std::filesystem::directory_iterator(LAS_PATH)) {
		if (std::filesystem::is_regular_file(entry.path())) {
			std::string fn = entry.path().filename().string();

			if ((DEBUG == 1 || SELECT_METHOD == 1) && fn.compare(selectLas) != 0)
				continue;

			//std::cout << entry.path().filename().string() << std::endl;
			ReaderLas readerLas(entry.path().string());
			readerLas.Read();

			std::vector<MyPoint>* points = readerLas.Get();

			m_lasData.insert({ fn, *points });

			points->clear();
			readerLas.Flush();
		}
	}
	std::cout << "Las Files Loaded"<< std::endl;

	ReaderCsv readerCsv;
	readerCsv.Read(ASSETS_PATH "compactBuildings.csv");

	std::vector<std::string> lines = readerCsv.Ottieni();

	std::vector<std::shared_ptr<Building>> buildings;
	
	if (SELECT_METHOD == 0) {
		for (std::string line : lines) {
			std::shared_ptr<Building> building = BuildingFactory::CreateBuilding(line);
			if (building->GetCodiceOggetto() == select) {
				buildings.push_back(building);
			}
		}
	}
	else {
		for (std::string line : lines) {
			std::shared_ptr<Building> building = BuildingFactory::CreateBuilding(line);
			buildings.push_back(building);
		}
	}

	std::cout << "Buildings Loaded" << std::endl;

	std::cout << "### Computation Start" << std::endl;

	readerCsv.Flush();
	std::vector<MyMesh> meshes;
	std::string filePath(OUTPUT_PATH "temp.stl");
	if (SELECT_METHOD == 1)
		filePath = OUTPUT_PATH + selectLas.substr(0, selectLas.size()-4) + "_roofs.stl";

	auto start = std::chrono::high_resolution_clock::now();

#if DEBUG
	int c = 0;
	int tot = 0;

	std::chrono::duration<double> time_acc_filter(0);
	std::chrono::duration<double> time_acc_cluster(0);
	std::chrono::duration<double> time_acc_grid(0);
	std::chrono::duration<double> time_acc_edge(0);
	std::chrono::duration<double> time_acc_triangU(0);
	std::chrono::duration<double> time_acc_ext(0);
	std::chrono::duration<double> time_acc_poly(0);
	std::chrono::duration<double> time_acc_feat(0);
	std::chrono::duration<double> time_acc_prune(0);
	std::chrono::duration<double> time_acc_triangC(0);
	std::chrono::duration<double> time_acc_coord(0);

	for (std::shared_ptr<Building> building : buildings) {
		std::vector<MyPoint> targetPoints;
		c++;

		auto st = std::chrono::high_resolution_clock::now();
		
		auto geomFactory = geos::geom::GeometryFactory::create();
		
		std::vector<std::string> tiles = building->GetTiles();
		int found = 0;

		for (const std::string& tile : tiles) {
			auto pos = m_lasData.find(tile);
			if (pos != m_lasData.end()) {
				found++;

				std::vector<MyPoint> points = pos->second;
				if (!points.empty()) {
					for (auto& p : points) {
						if (p.z >= building->GetQuotaGronda() && p.z <= (building->GetQuotaGronda() + building->GetTolleranza())) {
							auto point = geomFactory->createPoint(geos::geom::Coordinate(p.x, p.y, p.z));
							if (building->GetPolygon()->contains(point.get())) {
								targetPoints.push_back(p);
							}
						}
					}
				}
			}
		}
		if (found == 0)
			continue;
		if (targetPoints.size() <= 20)
			continue;

		if (SHOW_STEPS) {
			UtilsCV::ShowPoints(targetPoints, 2.0, 10.0, false);
		}

		time_acc_filter += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		tot++;
		int buildingCornerNumb = building->GetPolygon()->getNumPoints() - 1;
		if (PRINT_TEMP) {
			std::cout << "Edificio: " << c << "/" << buildings.size() << std::endl;
			std::cout << "##### Cod. Oggetto: " << building->GetCodiceOggetto() << std::endl;
			std::cout << "Corners number: " << buildingCornerNumb << std::endl;
			std::cout << "Points found: " << targetPoints.size() << std::endl;
		}

		st = std::chrono::high_resolution_clock::now();

		std::vector<MyPoint> mainCluster = Dbscan::GetMainCluster(std::span(targetPoints), 0.8, 10);

		time_acc_cluster += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		if (mainCluster.size() == 0)
			continue;
		
		st = std::chrono::high_resolution_clock::now();

		Grid grid;
		grid.Init(mainCluster, 0.1, 2.0, 0.2);
		grid.FillHoles(3, 3);
		targetPoints.clear();

		time_acc_grid += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		st = std::chrono::high_resolution_clock::now();

		std::vector<std::vector<float>> br = grid.GetBooleanRoof();

		std::shared_ptr<ImageProcesser> roofEdgeProcesser = ImageProcesserFactory::CreateEdgePipeline(br, SHOW_STEPS);

		float safetyFactor = 3.0;
		roofEdgeProcesser->Process(buildingCornerNumb*safetyFactor);
		std::vector<MyPoint2> roofResult = roofEdgeProcesser->GetOutput();

		time_acc_edge += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		if (roofResult.size() < 3) // can't triangulate
			continue;

		st = std::chrono::high_resolution_clock::now();

		TriangleWrapper triWrap;
		triWrap.Initialize();
		triWrap.UploadPoints(roofResult);
		std::vector<MyTriangle2> tris2 = triWrap.Triangulate();
		std::vector<std::vector<int>> indices = triWrap.GetIndices();

		time_acc_triangU += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		cv::Scalar delaunay_color(224, 9, 198);
		cv::Scalar point_ext(224, 183, 74);
		cv::Scalar point_ext_d(38, 135, 224);
		cv::Scalar point_ridge(45, 224, 134);

		if (SHOW_STEPS) {
			cv::Mat resImage = cv::Mat(cv::Size(br.size(), br[0].size()), CV_MAKETYPE(CV_8U, 3), cv::Scalar(255, 255, 255));

			for (int i = 0; i < tris2.size(); i++) {
				cv::line(resImage, cv::Point(tris2[i].p1.x, tris2[i].p1.y), cv::Point(tris2[i].p2.x, tris2[i].p2.y), delaunay_color, 1);
				cv::line(resImage, cv::Point(tris2[i].p2.x, tris2[i].p2.y), cv::Point(tris2[i].p3.x, tris2[i].p3.y), delaunay_color, 1);
				cv::line(resImage, cv::Point(tris2[i].p1.x, tris2[i].p1.y), cv::Point(tris2[i].p3.x, tris2[i].p3.y), delaunay_color, 1);
			}

			for (size_t i = 0; i < roofResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(roofResult[i].x)), static_cast<int>(std::round(roofResult[i].y)));
				circle(resImage, temp, 1, point_ext, 2); // BGR
			}

			UtilsCV::Show(resImage);
		}

		st = std::chrono::high_resolution_clock::now();

		std::vector<MyTriangle> tempTris;
		cv::Mat cFill = roofEdgeProcesser->GetCFill();
		for (int i = 0; i < tris2.size(); /* no increment here */) {
			MyTriangle2 tri2 = tris2[i];
			float c_x = (tri2.p1.x + tri2.p2.x + tri2.p3.x) / 3.0;
			float c_y = (tri2.p1.y + tri2.p2.y + tri2.p3.y) / 3.0;

			if (cFill.at<uchar>(c_y, c_x) != 0) {
				MyPoint p1 = grid.GetGridPointCoord(tri2.p1.x, tri2.p1.y);
				MyPoint p2 = grid.GetGridPointCoord(tri2.p2.x, tri2.p2.y);
				MyPoint p3 = grid.GetGridPointCoord(tri2.p3.x, tri2.p3.y);

				tempTris.push_back({ p1, p2, p3 });

				++i; // only increment if we didn't erase
			}
			else {
				// remove this entry from tris2
				tris2.erase(tris2.begin() + i);
				indices.erase(indices.begin() + i);
			}
		}

		if (SHOW_STEPS) {
			cv::Mat resImage = cv::Mat(cv::Size(br.size(), br[0].size()), CV_MAKETYPE(CV_8U, 3), cv::Scalar(255, 255, 255));

			for (int i = 0; i < tris2.size(); i++) {
				cv::line(resImage, cv::Point(tris2[i].p1.x, tris2[i].p1.y), cv::Point(tris2[i].p2.x, tris2[i].p2.y), delaunay_color, 1);
				cv::line(resImage, cv::Point(tris2[i].p2.x, tris2[i].p2.y), cv::Point(tris2[i].p3.x, tris2[i].p3.y), delaunay_color, 1);
				cv::line(resImage, cv::Point(tris2[i].p1.x, tris2[i].p1.y), cv::Point(tris2[i].p3.x, tris2[i].p3.y), delaunay_color, 1);
			}

			for (size_t i = 0; i < roofResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(roofResult[i].x)), static_cast<int>(std::round(roofResult[i].y)));
				circle(resImage, temp, 1, point_ext, 2); // BGR
			}

			UtilsCV::Show(resImage);
		}

		std::map<std::pair<int, int>, int> edgeFrequency;

		for (const auto& triangle : indices) {
			for (int i = 0; i < 3; ++i) {
				// need to order the vertices
				std::pair<int, int> edge = std::minmax(triangle[i], triangle[(i + 1) % 3]);
				// increment frequency
				edgeFrequency[edge]++;
			}
		}

		std::list<std::pair<int, int>> externalEdges;

		for (const auto& item : edgeFrequency) {
			if (item.second == 1) {
				externalEdges.push_back(item.first);
			}
		}

		time_acc_ext += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);
		
		//std::cout << "number of ext edges: " << externalEdges.size() << std::endl;

		if (SHOW_CLEAN_EDGES) {
			cv::Mat resImage = cv::Mat(cv::Size(br.size(), br[0].size()), CV_MAKETYPE(CV_8U, 3), cv::Scalar(255,255,255));

			for (const auto& edge : externalEdges) {
				cv::Point pt1(roofResult[edge.first].x, roofResult[edge.first].y);
				cv::Point pt2(roofResult[edge.second].x, roofResult[edge.second].y);

				cv::line(resImage, pt1, pt2, delaunay_color, 1);
			}

			for (size_t i = 0; i < roofResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(roofResult[i].x)), static_cast<int>(std::round(roofResult[i].y)));
				circle(resImage, temp, 1, point_ext, 2); // BGR
			}

			UtilsCV::Show(resImage);
		}

		st = std::chrono::high_resolution_clock::now();

		int precisionVal = buildingCornerNumb * 1.8;
		if (precisionVal > externalEdges.size())
			precisionVal = externalEdges.size();

		//std::cout << "prec val: " << precisionVal << std::endl;

		/*
		for (const auto& edge : externalEdges) {
			std::cout << "edge: " << edge.first << " - " << edge.second << std::endl;
		}
		*/

		std::list<std::pair<int, int>> cleanEdges;

		//std::cout << "edges before: " << externalEdges.size() << std::endl;

		while (!externalEdges.empty()) { // no increment here
			auto curr = externalEdges.begin();
			if (curr->first < precisionVal) {
				if (curr->second < precisionVal) {
					cleanEdges.push_back(*curr);
					externalEdges.erase(curr);  // delete this entry from external edges and move to next
				}
				else {
					int temp = UtilsTriangle::FindPrimaryVert(externalEdges, curr->first, precisionVal);
					if (temp > 0 && temp != curr->first) {
						cleanEdges.push_back({ curr->first, temp });
					}
				}
			}
			else {
				break;
			}
		}
		/*
		std::cout << "edges after: " << externalEdges.size() << std::endl;

		for (const auto& edge : cleanEdges) {
			std::cout << "edge: " << edge.first << " - " << edge.second << std::endl;
		}
		*/
		//std::cout << "clean edges done: " << building->GetCodiceOggetto() << std::endl;

		if (SHOW_CLEAN_EDGES) {
			cv::Mat resImage = cv::Mat(cv::Size(br.size(), br[0].size()), CV_8UC3, cv::Scalar(255,255,255));
			for (const auto& edge : cleanEdges) {
				cv::Point pt1(roofResult[edge.first].x, roofResult[edge.first].y);
				cv::Point pt2(roofResult[edge.second].x, roofResult[edge.second].y);

				cv::line(resImage, pt1, pt2, delaunay_color, 1);
			}

			for (size_t i = 0; i < roofResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(roofResult[i].x)), static_cast<int>(std::round(roofResult[i].y)));
				
				if (i < precisionVal)
					circle(resImage, temp, 1, point_ext, 2); // BGR
				else 
					circle(resImage, temp, 1, point_ext_d, 2); // BGR
			}

			UtilsCV::Show(resImage);
		}

		std::vector<std::pair<int, int>> outEdge = UtilsTriangle::Polygonize(cleanEdges);

		time_acc_poly += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		roofResult.erase(roofResult.begin() + precisionVal, roofResult.end());

		st = std::chrono::high_resolution_clock::now();

		std::vector<std::vector<float>> lm = grid.GetLocalMax(11);
		std::shared_ptr<ImageProcesser> ridgeEdgeProcesser = ImageProcesserFactory::CreateRidgePipeline(lm, SHOW_STEPS);
		ridgeEdgeProcesser->Process();
		std::vector<MyPoint2> ridgeResult = ridgeEdgeProcesser->GetOutput();

		time_acc_feat += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);
		st = std::chrono::high_resolution_clock::now();

		for (auto it1 = ridgeResult.begin(); it1 != ridgeResult.end(); ++it1) {
			for (auto it2 = roofResult.begin(); it2 != roofResult.end(); ++it2) {
				float dist = it1->distance(*it2);

				if (dist < 2.0) {
					it1 = ridgeResult.erase(it1);
					--it1;
					break;
				}
			}
		}
		time_acc_prune += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		st = std::chrono::high_resolution_clock::now();

		triWrap.Initialize();
		triWrap.UploadPoints(roofResult, ridgeResult, outEdge);
		std::vector<MyTriangle2> triangles2 = triWrap.TriangulateConstrained();

		time_acc_triangC += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		st = std::chrono::high_resolution_clock::now();

		std::vector<MyTriangle> triangles;
		for (int i = 0; i < triangles2.size(); ++i) {
			MyTriangle2 tri2 = triangles2[i];
			
			MyPoint p1 = grid.GetGridPointCoord(tri2.p1.x, tri2.p1.y);
			MyPoint p2 = grid.GetGridPointCoord(tri2.p2.x, tri2.p2.y);
			MyPoint p3 = grid.GetGridPointCoord(tri2.p3.x, tri2.p3.y);

			triangles.push_back({ p1, p2, p3 });
		}

		time_acc_coord += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - st);

		meshes.push_back(MyMesh(triangles));

		if (PRINT_TEMP)
			std::cout << "Mesh Done: " << building->GetCodiceOggetto() << std::endl;

		if (SHOW_RESULT) {
			cv::Mat resImage = cv::Mat(cv::Size(br.size(), br[0].size()), CV_8UC3, cv::Scalar(255,255,255));
			for (const auto& triangle : triangles2) {
				cv::Point pt1(triangle.p1.x, triangle.p1.y);
				cv::Point pt2(triangle.p2.x, triangle.p2.y);
				cv::Point pt3(triangle.p3.x, triangle.p3.y);

				cv::line(resImage, pt1, pt2, delaunay_color, 1.5);
				cv::line(resImage, pt2, pt3, delaunay_color, 1.5);
				cv::line(resImage, pt3, pt1, delaunay_color, 1.5);
			}

			for (size_t i = 0; i < roofResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(roofResult[i].x)), static_cast<int>(std::round(roofResult[i].y)));
				circle(resImage, temp, 1, point_ext, 2); // BGR
			}
			for (size_t i = 0; i < ridgeResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(ridgeResult[i].x)), static_cast<int>(std::round(ridgeResult[i].y)));
				circle(resImage, temp, 1, point_ridge, 2); // BGR
			}

			UtilsCV::Show(resImage);
		}
	}
	std::cout << "### Number of shapes: " << tot << "\n" << std::endl;

	if (tot > 0) {
		auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);

		double avgT = (dur.count()) / tot;
		double avgTimeFilter = (time_acc_filter.count() * 1000) / tot;
		double avgTimeCluster = (time_acc_cluster.count() * 1000) / tot;
		double avgTimeGrid = (time_acc_grid.count() * 1000) / tot;
		double avgTimeTriangU = (time_acc_triangU.count() * 1000) / tot;
		double avgTimeExtFilt = (time_acc_ext.count() * 1000) / tot;
		double avgTimePoly = (time_acc_poly.count() * 1000) / tot;
		double avgTimeFeat = (time_acc_feat.count() * 1000) / tot;
		double avgTimePrune = (time_acc_prune.count() * 1000) / tot;
		double avgTimeTriangC = (time_acc_triangC.count() * 1000) / tot;
		double avgTimeCoord = (time_acc_coord.count() * 1000) / tot;

		double total = avgT;
		double percTimeFilter = (avgTimeFilter / total) * 100;
		double percTimeCluster = (avgTimeCluster / total) * 100;
		double percTimeGrid = (avgTimeGrid / total) * 100;
		double percTimeTriangU = (avgTimeTriangU / total) * 100;
		double percTimeExtFilt = (avgTimeExtFilt / total) * 100;
		double percTimePoly = (avgTimePoly / total) * 100;
		double percTimeFeat = (avgTimeFeat / total) * 100;
		double percTimePrune = (avgTimePrune / total) * 100;
		double percTimeTriangC = (avgTimeTriangC / total) * 100;
		double percTimeCoord = (avgTimeCoord / total) * 100;

		std::cout << "### Avg. Tot time: " << avgT << " ms\n\n";

		std::cout << "### Avg. Time filter: " << avgTimeFilter << " ms (" << percTimeFilter << "%)\n";
		std::cout << "### Avg. Time cluster: " << avgTimeCluster << " ms (" << percTimeCluster << "%)\n";
		std::cout << "### Avg. Time grid: " << avgTimeGrid << " ms (" << percTimeGrid << "%)\n";
		std::cout << "### Avg. Time triang unconstrained: " << avgTimeTriangU << " ms (" << percTimeTriangU << "%)\n";
		std::cout << "### Avg. Time ext filt: " << avgTimeExtFilt << " ms (" << percTimeExtFilt << "%)\n";
		std::cout << "### Avg. Time poly: " << avgTimePoly << " ms (" << percTimePoly << "%)\n";
		std::cout << "### Avg. Time feat: " << avgTimeFeat << " ms (" << percTimeFeat << "%)\n";
		std::cout << "### Avg. Time prune: " << avgTimePrune << " ms (" << percTimePrune << "%)\n";
		std::cout << "### Avg. Time triang constrained: " << avgTimeTriangC << " ms (" << percTimeTriangC << "%)\n";
		std::cout << "### Avg. Time coord: " << avgTimeCoord << " ms (" << percTimeCoord << "%)\n\n";
	}

#else
	int size = buildings.size();
	int c = 0;
	std::for_each(std::execution::par, buildings.begin(), buildings.end(), [&c, &size, &meshes, this](std::shared_ptr<Building> building) {
		try {
			std::vector<MyPoint> targetPoints;
			++c;

			/*
			int perc = (c * 100) / size;

			if (perc % 25 == 0 && perc != 0) {
				std::cout << perc << "%" << std::endl;
			}
			*/

			auto geomFactory = geos::geom::GeometryFactory::create();

			std::vector<std::string> tiles = building->GetTiles();
			int found = 0;

			for (const std::string& tile : tiles) {
				auto pos = m_lasData.find(tile);
				if (pos != m_lasData.end()) {
					found++;

					std::vector<MyPoint> points = pos->second;
					if (!points.empty()) {
						for (auto& p : points) {
							if (p.z >= building->GetQuotaGronda() && p.z <= (building->GetQuotaGronda() + building->GetTolleranza())) {
								auto point = geomFactory->createPoint(geos::geom::Coordinate(p.x, p.y, p.z));
								if (building->GetPolygon()->contains(point.get())) {
									targetPoints.push_back(p);
								}
							}
						}
					}
				}
			}
			if (found == 0)
				return;
			if (targetPoints.size() <= 20)
				return;

			int buildingCornerNumb = building->GetPolygon()->getNumPoints() - 1;

			std::vector<MyPoint> mainCluster = Dbscan::GetMainCluster(std::span(targetPoints), 0.8, 10);

			if (mainCluster.size() == 0)
				return;

			Grid grid;
			grid.Init(mainCluster, 0.1, 2.0, 0.2);
			grid.FillHoles(3, 3);
			targetPoints.clear();

			std::vector<std::vector<float>> br = grid.GetBooleanRoof();

			std::shared_ptr<ImageProcesser> roofEdgeProcesser = ImageProcesserFactory::CreateEdgePipeline(br, SHOW_STEPS);

			float safetyFactor = 3.0;
			roofEdgeProcesser->Process(buildingCornerNumb * safetyFactor);
			std::vector<MyPoint2> roofResult = roofEdgeProcesser->GetOutput();

			if (roofResult.size() < 3) // can't triangulate
				return;

			TriangleWrapper triWrap;
			triWrap.Initialize();
			triWrap.UploadPoints(roofResult);
			std::vector<MyTriangle2> tris2 = triWrap.Triangulate();
			std::vector<std::vector<int>> indices = triWrap.GetIndices();

			std::vector<MyTriangle> tempTris;
			cv::Mat cFill = roofEdgeProcesser->GetCFill();
			for (int i = 0; i < tris2.size(); /* no increment here */) {
				MyTriangle2 tri2 = tris2[i];
				float c_x = (tri2.p1.x + tri2.p2.x + tri2.p3.x) / 3.0;
				float c_y = (tri2.p1.y + tri2.p2.y + tri2.p3.y) / 3.0;

				if (cFill.at<uchar>(c_y, c_x) != 0) {
					MyPoint p1 = grid.GetGridPointCoord(tri2.p1.x, tri2.p1.y);
					MyPoint p2 = grid.GetGridPointCoord(tri2.p2.x, tri2.p2.y);
					MyPoint p3 = grid.GetGridPointCoord(tri2.p3.x, tri2.p3.y);

					tempTris.push_back({ p1, p2, p3 });

					++i; // only increment if we didn't erase
				}
				else {
					// remove this entry from tris2
					tris2.erase(tris2.begin() + i);
					indices.erase(indices.begin() + i);
				}
			}

			std::map<std::pair<int, int>, int> edgeFrequency;

			for (const auto& triangle : indices) {
				for (int i = 0; i < 3; ++i) {
					// need to order the vertices
					std::pair<int, int> edge = std::minmax(triangle[i], triangle[(i + 1) % 3]);
					// increment frequency
					edgeFrequency[edge]++;
				}
			}

			std::list<std::pair<int, int>> externalEdges;

			for (const auto& item : edgeFrequency) {
				if (item.second == 1) {
					externalEdges.push_back(item.first);
				}
			}


			int precisionVal = buildingCornerNumb * 1.8;
			if (precisionVal > externalEdges.size())
				precisionVal = externalEdges.size();


			std::list<std::pair<int, int>> cleanEdges;

			while (!externalEdges.empty()) { // no increment here
				auto curr = externalEdges.begin();
				if (curr->first < precisionVal) {
					if (curr->second < precisionVal) {
						cleanEdges.push_back(*curr);
						externalEdges.erase(curr);  // delete this entry from external edges and move to next
					}
					else {
						int temp = UtilsTriangle::FindPrimaryVert(externalEdges, curr->first, precisionVal);
						if (temp > 0 && temp != curr->first) {
							cleanEdges.push_back({ curr->first, temp });
						}
					}
				}
				else {
					break;
				}
			}

			roofResult.erase(roofResult.begin() + precisionVal, roofResult.end());

			std::vector<std::vector<float>> lm = grid.GetLocalMax(11);
			std::shared_ptr<ImageProcesser> ridgeEdgeProcesser = ImageProcesserFactory::CreateRidgePipeline(lm, SHOW_STEPS);
			ridgeEdgeProcesser->Process();
			std::vector<MyPoint2> ridgeResult = ridgeEdgeProcesser->GetOutput();

			for (auto it1 = ridgeResult.begin(); it1 != ridgeResult.end(); ++it1) {
				for (auto it2 = roofResult.begin(); it2 != roofResult.end(); ++it2) {
					float dist = it1->distance(*it2);

					if (dist < 2.0) {
						it1 = ridgeResult.erase(it1);
						--it1;
						break;
					}
				}
			}

			std::vector<std::pair<int, int>> outEdge = UtilsTriangle::Polygonize(cleanEdges);

			triWrap.Initialize();
			triWrap.UploadPoints(roofResult, ridgeResult, outEdge);
			std::vector<MyTriangle2> triangles2 = triWrap.TriangulateConstrained();

			std::vector<MyTriangle> triangles;
			for (int i = 0; i < triangles2.size(); ++i) {
				MyTriangle2 tri2 = triangles2[i];

				MyPoint p1 = grid.GetGridPointCoord(tri2.p1.x, tri2.p1.y);
				MyPoint p2 = grid.GetGridPointCoord(tri2.p2.x, tri2.p2.y);
				MyPoint p3 = grid.GetGridPointCoord(tri2.p3.x, tri2.p3.y);

				triangles.push_back({ p1, p2, p3 });
			}

			meshes.push_back(MyMesh(triangles));
		}
		catch (const std::exception& e) {
			std::cerr << "Error during execution of building: " << building->GetCodiceOggetto() << "\nError: " << e.what() << std::endl;
		}
	});
#endif

	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

	std::cout << "### Computation time: " << minutes.count() << " min and " << seconds.count() << " sec" << std::endl;

	Exporter::ExportStl(meshes, filePath);

	std::cout << "### Export done ###" << std::endl;
}
