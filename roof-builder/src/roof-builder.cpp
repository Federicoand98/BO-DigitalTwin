// roof-builder.cpp: definisce il punto di ingresso dell'applicazione.
//

#include "roof-builder.h"
#include <fstream>
#include <string>
#include <filesystem>
#include "Readers/ReaderLas.h"
#include "Grid.h"

#include <opencv2/opencv.hpp>

int main() {
	for (const auto& file : std::filesystem::directory_iterator(ASSETS_PATH "las/")) {
		try {
			ReaderLas readerLas(file.path().string());

			readerLas.Read();

			pdal::PointViewSet* pointViewSet = readerLas.Get();
			
			if (!pointViewSet->empty()) {
				pdal::PointViewPtr pointView = *pointViewSet->begin();
				for (pdal::PointId id = 0; id < 5; ++id) {
					double x = pointView->getFieldAs<double>(pdal::Dimension::Id::X, id);
					double y = pointView->getFieldAs<double>(pdal::Dimension::Id::Y, id);
					double z = pointView->getFieldAs<double>(pdal::Dimension::Id::Z, id);

					std::cout << x << " " << y << " " << z << std::endl;
				}
			}

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


	/*
	cv::Mat image = cv::imread(ASSETS_PATH "canny.png", cv::IMREAD_GRAYSCALE);

	cv::imshow("Immagine", image);

	cv::waitKey(0);
	*/
	

	return 0;
}
