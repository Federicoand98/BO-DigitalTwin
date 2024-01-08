// roof-builder.cpp: definisce il punto di ingresso dell'applicazione.
//

#include "roof-builder.h"
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include "Readers/ReaderLas.h"
#include "Grid.h"
#include <geos_c.h>

#include <opencv2/opencv.hpp>

int main() {
	auto start = std::chrono::high_resolution_clock::now();
	for (const auto& file : std::filesystem::directory_iterator(ASSETS_PATH "las/")) {
		try {
			ReaderLas readerLas(file.path().string());

			readerLas.Read();

			std::vector<MyPoint>* points = readerLas.Get();
			
			if (!points->empty()) {
				for (auto &p : *points) {
					//std::cout << p.x << " " << p.y << " " << p.z << std::endl;
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
	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> diff = end - start;

    std::cout << "Tempo trascorso: " << diff.count() << " s\n";

	/*
	cv::Mat image = cv::imread(ASSETS_PATH "canny.png", cv::IMREAD_GRAYSCALE);

	cv::imshow("Immagine", image);

	cv::waitKey(0);
	*/
	

	return 0;
}
