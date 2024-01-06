#include "ReaderLas.h"

ReaderLas::ReaderLas(const std::string& filename) {
    options.add("filename", filename);
    
    reader = factory.createStage("readers.las");
    reader->setOptions(options);
}

void ReaderLas::Read() {
    reader->prepare(table);
    pointViewSet = reader->execute(table);
}

pdal::PointViewSet* ReaderLas::Get() {
    return &pointViewSet;
}

void ReaderLas::Flush() {
    options = pdal::Options();
    table.clearSpatialReferences();
    pointViewSet = pdal::PointViewSet();
}


ReaderLas::~ReaderLas() {
    //
}
