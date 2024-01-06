#include <pdal/Options.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>

#include <pdal/io/LasReader.hpp>
#include <pdal/PipelineManager.hpp>

class Reader {
public:
    virtual void Read() = 0;
    virtual void Flush() = 0;
    virtual pdal::PointViewSet* Get() = 0;
    virtual ~Reader() {}
};

class ReaderLas : public Reader {
private:
    pdal::Options options;
    pdal::StageFactory factory;
    pdal::Stage* reader;
    pdal::PointTable table;
    pdal::PointViewSet pointViewSet;

public:
    ReaderLas(const std::string& filename);
    void Read() override;
    pdal::PointViewSet* Get() override;
    void Flush() override;
    ~ReaderLas();
};
