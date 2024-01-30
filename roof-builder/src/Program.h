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
#include "Printer.h"
#include "Triangle/TriangleWrap.h"
#include "Exporter.h"

#define DEBUG 0

#define SHOW_RESULT false
#define SHOW_STEPS false
#define SHOW_CLEAN_EDGES false 

#define SELECT_METHOD 1 //0 single building, 1 single las, 2 all las in dir

#define LAS_PATH ASSETS_PATH "las/"

int findPrimaryVert(std::list<std::pair<int, int>>& v, int num, int t) {
	int pos = num;
	bool found = false;

	for (auto it = v.begin(); !found && it != v.end(); /* no increment here */) {
		//std::cout << "searching " << pos << std::endl;
		//std::cout << "current: " << it->first << " - " << it->second << std::endl;
		if (it->first == pos) {
			//std::cout << "found " << pos << " with " << it->second << std::endl;
			pos = it->second;

			// remove this entry from v
			it = v.erase(it);
			if (pos < t) {
				found = true;
				return pos;
			}
		}
		else if (it->second == pos) {
			//std::cout << "found " << pos << " with " << it->first << std::endl;
			pos = it->first;

			// remove this entry from v
			it = v.erase(it);
			if (pos < t) {
				found = true;
				return pos;
			}
		}
		else {
			++it; // only increment if we didn't erase
		}

		if (it == v.end()) {
			it = v.begin();
		}
	}

	return -1;
}

int findNextVert(std::list<std::pair<int, int>>& v, int num) {
	for (auto it = v.begin(); it != v.end(); /* no increment here */) {
		//std::cout << "searching " << num << std::endl;
		//std::cout << "current: " << it->first << " - " << it->second << std::endl;
		if (it->first == num) {
			int temp = it->second;
			v.erase(it);
			return temp;
		}
		else if (it->second == num) {
			int temp = it->first;
			it = v.erase(it);
			return temp;	
		}
		else {
			++it; // only increment if we didn't erase
		}
	}

	return -1;
}

std::vector<std::pair<int, int>> polygonize(std::list<std::pair<int, int>>& list) {
	std::vector<std::pair<int, int>> res;
	int init = list.front().first;
	int temp = init;
	while (!list.empty()) { // no increment here
		int sec = findNextVert(list, temp);
		res.push_back({ temp, sec });
		temp = sec;
		if (sec == init) { // there are some dirty edges
			return res;
		}
	}

	return res;
}

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

	//uint16_t select = 52578;
	//uint16_t select = 24069;

	uint16_t select = 33614;
	std::string selectLas = "32_686000_4929000.las";

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


#if DEBUG
	int c = 0;

	for (std::shared_ptr<Building> building : buildings) {
		std::vector<MyPoint> targetPoints;
		c++;
		
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

		std::cout << "Edificio: " << c << "/" << buildings.size() << std::endl;
		std::cout << "##### Cod. Oggetto: " << building->GetCodiceOggetto() << std::endl;

		int buildingCornerNumb = building->GetPolygon()->getNumPoints() - 1;
		std::cout << "Corners number: " << buildingCornerNumb << std::endl;

		std::cout << "Points found: " << targetPoints.size() << std::endl;

		std::vector<MyPoint> mainCluster = Dbscan::GetMainCluster(std::span(targetPoints), 0.8, 10);

		if (mainCluster.size() == 0)
			continue;

		Grid grid;
		grid.Init(mainCluster, 0.1, 2.0, 0.2);
		grid.FillHoles(3, 3);
		targetPoints.clear();

		std::vector<std::vector<float>> br = grid.GetBooleanRoof();

		std::shared_ptr<ImageProcesser> roofEdgeProcesser = ImageProcesserFactory::CreateEdgePipeline(br, SHOW_STEPS);

		float safetyFactor = 3.0;
		roofEdgeProcesser->Process(buildingCornerNumb*safetyFactor);
		std::vector<MyPoint2> roofResult = roofEdgeProcesser->GetOutput();

		if (roofResult.size() < 3) // can't triangulate
			continue;

		TriangleWrapper triWrap;
		triWrap.Initialize();
		triWrap.UploadPoints(roofResult);
		std::vector<MyTriangle2> tris2 = triWrap.Triangulate();
		std::vector<std::vector<int>> indices = triWrap.GetIndices();

		std::vector<MyTriangle> tempTris;
		for (int i = 0; i < tris2.size(); /* no increment here */) {
			MyTriangle2 tri2 = tris2[i];
			float c_x = (tri2.p1.x + tri2.p2.x + tri2.p3.x) / 3.0;
			float c_y = (tri2.p1.y + tri2.p2.y + tri2.p3.y) / 3.0;

			cv::Mat cFill = roofEdgeProcesser->GetCFill();
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

		//std::cout << "number of ext edges: " << externalEdges.size() << std::endl;

		
		if (SHOW_CLEAN_EDGES) {
			cv::Mat resImage = cv::Mat::zeros(cv::Size(br.size(), br[0].size()), CV_MAKETYPE(CV_8U, 3));
			for (size_t i = 0; i < roofResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(roofResult[i].x)), static_cast<int>(std::round(roofResult[i].y)));
				circle(resImage, temp, 0.1, cv::Scalar(0, 255, 0), 2); // BGR
			}

			cv::Scalar delaunay_color(128, 0, 128);

			for (const auto& edge : externalEdges) {
				cv::Point pt1(roofResult[edge.first].x, roofResult[edge.first].y);
				cv::Point pt2(roofResult[edge.second].x, roofResult[edge.second].y);

				cv::line(resImage, pt1, pt2, delaunay_color, 1);
			}
			UtilsCV::Show(resImage);
		}

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
					int temp = findPrimaryVert(externalEdges, curr->first, precisionVal);
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
			cv::Mat resImage = cv::Mat::zeros(cv::Size(br.size(), br[0].size()), CV_MAKETYPE(CV_8U, 3));
			for (size_t i = 0; i < roofResult.size(); i++) {
				cv::Point temp(static_cast<int>(std::round(roofResult[i].x)), static_cast<int>(std::round(roofResult[i].y)));
				circle(resImage, temp, 0.1, cv::Scalar(0, 255, 0), 2); // BGR
			}
			
			cv::Scalar delaunay_color(128, 0, 128);

			for (const auto& edge : cleanEdges) {
				cv::Point pt1(roofResult[edge.first].x, roofResult[edge.first].y);
				cv::Point pt2(roofResult[edge.second].x, roofResult[edge.second].y);

				cv::line(resImage, pt1, pt2, delaunay_color, 1);
			}
			UtilsCV::Show(resImage);
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

		std::vector<std::pair<int, int>> outEdge = polygonize(cleanEdges);

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
		std::cout << "Mesh Done: " << building->GetCodiceOggetto() << std::endl;

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
#else
	int size = buildings.size();
	int c = 0;

	std::for_each(std::execution::par, buildings.begin(), buildings.end(), [&c, &size, &meshes, this](std::shared_ptr<Building> building) {
		try {
			std::vector<MyPoint> targetPoints;
			c++;

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
			for (int i = 0; i < tris2.size(); /* no increment here */) {
				MyTriangle2 tri2 = tris2[i];
				float c_x = (tri2.p1.x + tri2.p2.x + tri2.p3.x) / 3.0;
				float c_y = (tri2.p1.y + tri2.p2.y + tri2.p3.y) / 3.0;

				cv::Mat cFill = roofEdgeProcesser->GetCFill();
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
						int temp = findPrimaryVert(externalEdges, curr->first, precisionVal);
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

			std::vector<std::pair<int, int>> outEdge = polygonize(cleanEdges);

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

	Exporter::ExportStl(meshes, filePath);

}
