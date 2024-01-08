#include "ReaderLas.h"

ReaderLas::ReaderLas(const std::string& filename) {
    options.add("filename", filename);
    
    reader = factory.createStage("readers.las");
    reader->setOptions(options);
}

void ReaderLas::Read() {
    reader->prepare(table);
    pointViewSet = reader->execute(table);

    if (!pointViewSet.empty()) {
        pdal::PointViewPtr pointView = *pointViewSet.begin();

        for (pdal::PointId id = 0; id < pointView->size(); ++id) {
            float x = pointView->getFieldAs<float>(pdal::Dimension::Id::X, id);
            float y = pointView->getFieldAs<float>(pdal::Dimension::Id::Y, id);
            float z = pointView->getFieldAs<float>(pdal::Dimension::Id::Z, id);
            
            points.push_back(MyPoint(x, y, z));
        }
    }
}

std::vector<MyPoint>* ReaderLas::Get() {
    return &points;
}

pdal::PointViewSet* ReaderLas::GetPVS() {
    return &pointViewSet;
}

void ReaderLas::Flush() {
    options = pdal::Options();
    table.clearSpatialReferences();
    pointViewSet.clear();
    points.clear();
}


ReaderLas::~ReaderLas() {
    //
}
