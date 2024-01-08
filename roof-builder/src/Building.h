#pragma once

#include <iostream>
#include <vector>
#include "MyPoint.h"

class BuildingFactory {
public:
	static std::shared_ptr<Building> createBuilding(const std::string& entry) {
		std::string token;
		std::istringstream tokenStream(entry);

		uint16_t codiceOggetto;
		float quotaGronda;
		float tolleranza;
		std::vector<MyPoint> points;
		int i = 0;

		while (std::getline(tokenStream, token, ';')) {
			if (i == 0) {
				codiceOggetto = std::stoi(token);
			}
			else if (i == 1) {
				quotaGronda = std::stof(token);
			}
			else if (i == 2) {
				points = getPoints(token);
			}
			else {
				tolleranza = std::stof(token);
			}

			i++;
		}

		return std::make_shared<Building>(codiceOggetto, quotaGronda, points, tolleranza);
	}

	static std::vector<Point> getPoints(const std::string& entry) {
		std::string newToken;
		std::istringstream pointStream(entry);
		std::vector<Point> points;

		while (std::getline(pointStream, newToken, ']')) {

			size_t pos;
			while ((pos = newToken.find('[')) != std::string::npos) {
				newToken.erase(pos, 1);
			}

			if (newToken.substr(0, 2) == ", ") {
				newToken.erase(0, 2);
			}

			std::istringstream pointstring(newToken);
			std::string token;
			std::vector<float> res;
			
			while (std::getline(pointstring, token, ',')) {
				res.push_back(std::stof(token));
			}

			points.push_back({ res.at(0), res.at(1), 0.0 });
			std::cout << points.size() << std::endl;
		}

		return points;
	}
};

class Building {
public:
	Building(uint16_t codice, float quota, std::vector<Point> shape, float toll) : m_CodiceOggetto(codice), m_QuotaGronda(quota), m_GeoShapePoints(shape), m_Tolleranza(toll) {}
	~Building() {}

private:
	uint16_t m_CodiceOggetto;
	float m_QuotaGronda;
	std::vector<Point> m_GeoShapePoints;
	float m_Tolleranza;
};
