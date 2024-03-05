#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <locale>
#include <iomanip>
#include <geos.h>
#include "MyPoint.h"


class Building {

public:
	Building(uint16_t codice, float quota, std::vector<geos::geom::Coordinate> shape, float toll, std::unique_ptr<geos::geom::Polygon> polygon, std::vector<std::string> tiles) : m_CodiceOggetto(codice), m_QuotaGronda(quota), m_GeoShapePoints(shape), m_Tolleranza(toll), m_Polygon(std::move(polygon)), m_Tiles(tiles){}
	~Building() {}

	uint16_t GetCodiceOggetto() const { return m_CodiceOggetto; }
	float GetQuotaGronda() const { return m_QuotaGronda; }
	float GetTolleranza() const { return m_Tolleranza; }
	std::vector<geos::geom::Coordinate> GetGeoShapePoints() const { return m_GeoShapePoints; }
	std::shared_ptr<geos::geom::Polygon> GetPolygon() { return m_Polygon; }
	std::vector<std::string> GetTiles() const { return m_Tiles; }

	std::vector<std::pair<MyPoint, MyPoint>> GetEdges() const {
		std::vector<std::pair<MyPoint, MyPoint>> edges;
		if (m_GeoShapePoints.empty()) return edges;

		for (size_t i = 0; i < m_GeoShapePoints.size(); ++i) {
			const auto& p1 = m_GeoShapePoints[i];
			const auto& p2 = m_GeoShapePoints[(i + 1) % m_GeoShapePoints.size()];

			edges.emplace_back(MyPoint(p1.x, p1.y, 0), MyPoint(p2.x, p2.y, 0));
		}

		return edges;
	}

private:
	uint16_t m_CodiceOggetto;
	float m_QuotaGronda;
	std::vector<std::string> m_Tiles;
	std::vector<geos::geom::Coordinate> m_GeoShapePoints;
	std::shared_ptr<geos::geom::Polygon> m_Polygon;
	float m_Tolleranza;
};


class BuildingFactory {

public:
	static std::shared_ptr<Building> CreateBuilding(const std::string& entry);

private:
	static std::vector<geos::geom::Coordinate> getPoints(const std::string& entry);
	static std::unique_ptr<geos::geom::Polygon> getPolygon(const std::vector<geos::geom::Coordinate>& coords);
	static std::vector<std::string> getTiles(const std::string& entry);
};
