#include <pdal/Options.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>

#include <pdal/io/LasReader.hpp>
#include <pdal/PipelineManager.hpp>

#include "Reader.h"
#include "../MyPoint.h"

class ReaderLas : public Reader<std::vector<MyPoint>> {
public:
    ReaderLas(const std::string& filename);
    void Read() override;
    void Read(const std::string& filePath) override;
    std::vector<MyPoint>* Get() override;
    void Flush() override;
    ~ReaderLas();

    pdal::PointViewSet* GetPVS();
private:
    pdal::Options options;
    pdal::StageFactory factory;
    pdal::Stage* reader;
    pdal::PointTable table;
    pdal::PointViewSet pointViewSet;
    std::vector<MyPoint> points;
};
