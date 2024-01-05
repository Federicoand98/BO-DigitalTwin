// roof-builder.cpp: definisce il punto di ingresso dell'applicazione.
//

#include "roof-builder.h"
#include <fstream>
#include <string>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/Options.hpp>
#include <pdal/PipelineManager.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/pdal_config.hpp>

#include <opencv2/opencv.hpp>

int main() {
	pdal::Options options;
	options.add("filename", ASSETS_PATH "32_684000_4930000.las");

	pdal::StageFactory f;
	pdal::Stage* reader = f.createStage("readers.las");

	reader->setOptions(options);

	pdal::PointTable table;
	reader->prepare(table);
	pdal::PointViewSet pointViewSet = reader->execute(table);

	/*
	if (!pointViewSet.empty()) {
		pdal::PointViewPtr pointView = *pointViewSet.begin();
		for (pdal::PointId id = 0; id < pointView->size(); ++id) {
			double x = pointView->getFieldAs<double>(pdal::Dimension::Id::X, id);
			double y = pointView->getFieldAs<double>(pdal::Dimension::Id::Y, id);
			double z = pointView->getFieldAs<double>(pdal::Dimension::Id::Z, id);

			std::cout << x << " " << y << " " << z << std::endl;
		}
	}
	*/

	cv::Mat image = cv::imread(ASSETS_PATH "canny.png", cv::IMREAD_GRAYSCALE);

	cv::imshow("Immagine", image);

	cv::waitKey(0);

	return 0;
}
